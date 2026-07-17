// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "FTGA_Grab.generated.h"

class UGameplayEffect;
class UFTCaptureEscapeComponent;
class AAIController;
class AFTCaptureDestination;

/**
 * [경비 전용] 잡기 어빌리티. 사거리 안의 대상을 확정으로 붙잡아 경비 CapturePoint에 부착하고,
 * 가장 가까운 AFTCaptureDestination으로 이송한다.
 *  - 대상이 좌우 연타로 탈출 게이지를 다 채우면 → [성공] 대상 해방 + 자신(경비) 스턴
 *  - 목적지에 도달하면(도착 전 탈출 실패)      → [실패] 대상에게 데미지
 * 이송 이동(MoveTo)과 도달 감지를 이 어빌리티가 소유한다. StateTree의 잡기 노드는 이 어빌리티만 발동하고 대기해야 한다
 * (거기서 또 MoveTo를 돌리면 이송 이동과 충돌).
 */
UCLASS()
class PROJECTFT_API UFTGA_Grab : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_Grab();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 확정 캐치 사거리(cm). 발동 시 대상이 이 안에 있으면 무조건 잡는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float GrabRange = 200.0f;

	// [탈출 난이도 = 이 경비의 붙잡는 힘] 대상이 탈출하려면 채워야 하는 총 struggle 양.
	// 대상의 좌우 전환당 힘(UFTCaptureEscapeComponent::StruggleGainPerFlip)으로 이만큼 쌓으면 탈출.
	// 예) 임계값 17 vs 전환당 힘 1.0 → 약 17번 전환. 값이 클수록 탈출이 어렵다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.01"))
	float EscapeThreshold = 17.0f;

	// [AI의 탈출 저지력] 초당 누적 struggle을 되끌어내리는 양. 대상의 자연증가(StrugglePassiveGainPerSecond)와
	// 매 틱 힘싸움을 벌인다. 감소 > 증가면 가만히 있으면 게이지가 빠지고, 반대면 저절로 찬다. 캡처 시작 시 컴포넌트에 주입.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float EscapeDecayPerSecond = 2.0f;

	// 탈출 실패(목적지 도달) 시 대상에게 줄 피해량.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float FailDamage = 100.0f;

	// 잡았을 때 대상에게 줄 피해량.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float CaptureStartDamage = 20.0f;

	// 잡고있는 중에 대상에게 초당 줄 피해량.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float CaptureDamagePerSecond = 1.0f;

	// 탈출 성공 시 자신(경비)에게 거는 스턴 지속시간(초).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float EscapeStunDuration = 3.0f;

	// 레벨에 목적지가 없거나 경로가 막혔을 때의 안전 제한시간(초). 이 시간 뒤 실패 처리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float FallbackCaptureSeconds = 8.0f;

	// 실패 시 대상에게 적용할 데미지 GE(SetByCaller Data.Damage). 기본 UFTGE_Damage.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 탈출 성공 시 자신에게 적용할 스턴 GE(SetByCaller Data.StunDuration). 기본 UFTGE_Stun.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	// 대상이 탈출 게이지를 다 채움 → 성공.
	UFUNCTION()
	void OnTargetEscaped();

	// 이송 MoveTo 완료. 목적지 도달(Success)이면 실패로 판정.
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	// 목적지/컨트롤러가 없을 때의 안전 타이머 → 실패.
	void OnFallbackTimeout();

	// 붙잡은 보안요원이 스턴/행동불능 상태가 되면 잡기를 즉시 해제한다.
	void OnOwnerImmobilizedTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 성공(bEscaped=true)/실패(false) 공통 마무리. 최초 1회만 효과 적용 후 해방·종료.
	void FinishGrab(bool bEscaped);
	void BroadcastCaptureMessage(FGameplayTag Channel) const;
	// 붙잡힌 대상에게 지정한 피해량을 적용한다.
	void ApplyCaptureDamage(float DamageAmount);
	// 붙잡혀 있는 동안 타이머로 반복 호출되어 초당 피해를 적용한다.
	void ApplyCaptureTickDamage();

	AFTCaptureDestination* FindNearestCaptureDestination(const FVector& From) const;

	TWeakObjectPtr<AActor> CapturedTarget;
	TWeakObjectPtr<UFTCaptureEscapeComponent> TargetEscapeComp;
	TWeakObjectPtr<AAIController> CachedAIController;
	FTimerHandle FallbackTimerHandle;
	FTimerHandle CaptureDamageTimerHandle;
	FDelegateHandle OwnerImmobilizedTagChangedHandle;
	bool bResolved = false;
	bool bBoundMoveCompleted = false;
	bool bCapturedMessageBroadcast = false;
	bool bEscapedMessageBroadcast = false;
};
