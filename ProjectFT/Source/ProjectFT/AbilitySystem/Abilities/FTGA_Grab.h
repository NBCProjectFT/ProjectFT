// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "FTGA_Grab.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UAnimMontage;
class UCharacterMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class USceneComponent;
class UFTCaptureEscapeComponent;
class AAIController;
class AFTCaptureDestination;
class AFTSecurityCharacter;

/**
 * [경비 전용] 잡기 어빌리티. 사거리 안의 대상을 확정으로 붙잡아 경비 몸(CaptureAttachSocketName 소켓)에 부착하고,
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

	void OpenGrabCaptureWindow();
	void CloseGrabCaptureWindow();

protected:
	// StateTree/AIController가 이미 잡기 시도 가능 거리를 판단한다. 이 값은 잘못된 외부 호출을 막는 안전 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float GrabRange = 300.0f;

	// [붙잡는 자세] 대상을 붙일 경비 메시의 소켓(본 이름도 가능). 잡기 종류마다 다른 자세를 쓸 수 있게 어빌리티가 소유한다
	// — 예) GA_Grab은 앞에 끌기 소켓, GA_GrabStrong은 어깨에 메기 소켓.
	//
	// 대상은 '루트(캡슐)'가 이 소켓에 스냅되므로 소켓 트랜스폼은 대상 캡슐 '중심'이 놓일 자리다(발밑 아님).
	// 캐릭터 메시는 캡슐 기준으로 이미 -90° yaw / Z -88쯤 틀어져 있어 소켓도 그 회전을 물려받는다 —
	// 대상이 90° 돌아가 보이면 소켓 회전을 스켈레톤 에디터에서 보정할 것(오프셋 튜닝 지점은 여기 하나뿐이다).
	//
	// 스켈레톤에 이 소켓이 없으면 경고 후 경비 CapturePoint(루트 기준)에 붙는다 — 소켓을 만들기 전까지는 종전 동작 그대로다.
	// 비워두면(NAME_None) 경고 없이 같은 폴백.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab")
	FName CaptureAttachSocketName = TEXT("Socket_Capture");

	// [탈출 난이도 = 이 경비의 붙잡는 힘] 대상이 탈출하려면 채워야 하는 총 struggle 양.
	// 대상의 좌우 전환당 힘(UFTCaptureEscapeComponent::StruggleGainPerFlip)으로 이만큼 쌓으면 탈출.
	// 예) 임계값 17 vs 전환당 힘 1.0 → 약 17번 전환. 값이 클수록 탈출이 어렵다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.01"))
	float EscapeThreshold = 17.0f;

	// [AI의 탈출 저지력] 초당 누적 struggle을 되끌어내리는 양. 대상의 자연증가(StrugglePassiveGainPerSecond)와
	// 매 틱 힘싸움을 벌인다. 감소 > 증가면 가만히 있으면 게이지가 빠지고, 반대면 저절로 찬다. 캡처 시작 시 컴포넌트에 주입.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float EscapeDecayPerSecond = 2.0f;

	// [붙잡힌 순간 1회] 붙잡히자마자 대상이 잃는 체력. 잡힌 것 자체의 고정 대가라 이후 탈출/이송 결과와 무관하게 한 번만 들어간다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float InitialCaptureDamage = 1.0f;

	// [붙잡힌 동안 지속] 대상이 초당 잃는 체력. 탈출 게이지의 힘싸움(자연증가 vs 저지력)과 같은 방식으로 매 틱 경과시간만큼
	// 쪼개 넣어 부드럽게 깎는다 — 체력은 float이고 HUD 체력바가 보간되므로 소수점 누적이 그대로 자연스럽게 보인다. 0이면 지속 피해 없음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float CaptureDamagePerSecond = 0.1f;

	// 지속 피해를 적용하는 간격(초). 이 간격마다 CaptureDamagePerSecond × 실제 경과시간을 적용하므로,
	// 간격을 바꿔도 총 피해량은 같고 '부드러움'만 달라진다(탈출 게이지 UFTGA_EscapableDebuff::EscapeTickInterval과 같은 기본값).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.01"))
	float CaptureDamageTickInterval = 0.05f;

	// 탈출 실패(목적지 도달) 시 대상에게 줄 피해량.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab", meta = (ClampMin = "0.0"))
	float FailDamage = 100.0f;

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
	
	// [붙잡힌 순간 1회] 초기 피해와 함께 적용할 '공격 표식' GE(에셋 태그 Effect.Hostile). 기본 UFTGE_Hostile.
	// 대상은 이 표식을 보고 피격 연출(피격음)과 어그로 신호 Event.Character.Attacked를 낸다.
	// 지속 피해 틱에는 일부러 붙이지 않는다 — 붙이면 틱마다 "공격당함"이 재발행돼 잡혀있는 내내 피격음이 울린다.
	// 비워두면 잡혀도 피격 연출/어그로 신호가 발생하지 않는다(표식 도입 전 동작).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab")
	TSubclassOf<UGameplayEffect> HostileMarkerEffectClass;

	// 잡기 시도 몽타주. NotifyState로 캡처 가능 구간을 연다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Animation")
	TObjectPtr<UAnimMontage> GrabMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Animation", meta = (ClampMin = "0.0"))
	float GrabMontagePlayRate = 1.0f;

	// Socket_Capture 위치 기준 성공 판정 반경(cm). 이 구 안에 PendingTarget Pawn이 있을 때만 bTargetCaptured 상태로 넘어간다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Confirm", meta = (ClampMin = "0.0"))
	float GrabConfirmRadius = 90.0f;

	// Montage/AnimSequence에 Notify가 빠져 있을 때 Ability가 영원히 남지 않게 하는 안전 여유 시간(초).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Confirm", meta = (ClampMin = "0.0"))
	float GrabAttemptTimeoutPadding = 0.25f;

	// Socket_Capture 판정 Sphere를 게임 화면에 표시한다. GA_Grab Blueprint에서 켜고 끌 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Debug")
	bool bDrawGrabCaptureDebugSphere = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Debug", meta = (ClampMin = "0.0"))
	float GrabCaptureDebugSphereLifeTime = 0.15f;

	// Grab 중에는 수동 위치 보정 대신 CharacterMovement를 유지하되 평소보다 느리게 움직인다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Movement", meta = (ClampMin = "0.0"))
	float GrabMovementSpeedScale = 0.8f;

	// Grab 중 급격한 방향 전환을 막기 위해 CharacterMovement RotationRate를 낮춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Grab|Movement", meta = (ClampMin = "0.0"))
	float GrabRotationRateScale = 0.35f;

private:
	// 대상이 탈출 게이지를 다 채움 → 성공.
	UFUNCTION()
	void OnTargetEscaped();

	// 이송 MoveTo 완료. 목적지 도달(Success)이면 실패로 판정.
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	void OnGrabCaptureSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 목적지/컨트롤러가 없을 때의 안전 타이머 → 실패.
	void OnFallbackTimeout();

	// 붙잡은 보안요원이 스턴/행동불능 상태가 되면 잡기를 즉시 해제한다.
	void OnOwnerImmobilizedTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 성공(bEscaped=true)/실패(false) 공통 마무리. 최초 1회만 효과 적용 후 해방·종료.
	void FinishGrab(bool bEscaped);
	void BroadcastCaptureMessage(FGameplayTag Channel) const;

	// 붙잡은 동안의 지속 피해 타이머 시작/1틱. CaptureDamagePerSecond가 0이면 아예 돌리지 않는다.
	void StartCaptureDamageTick();
	void TickCaptureDamage();

	// 대상에게 DamageEffectClass를 SetByCaller(음수 크기)로 적용한다. 초기 1회·지속·실패 피해가 모두 이 경로를 쓴다.
	// bMarkHostile=true면 피해 직후 HostileMarkerEffectClass를 함께 적용해 "공격당함"으로 표시한다(붙잡힌 순간 1회 전용).
	void ApplyDamageToTarget(float DamageAmount, bool bMarkHostile = false);

	// 대상을 붙일 지점을 정한다. CaptureAttachSocketName이 경비 스켈레톤에 있으면 (메시, 소켓명),
	// 없거나 비어 있으면 (CapturePoint, NAME_None)을 돌려준다 — 어느 쪽이든 부착 지점은 non-null이다.
	USceneComponent* ResolveCaptureAttachPoint(AFTSecurityCharacter* Security, FName& OutAttachSocketName) const;
	float PlayGrabMontage() const;
	void OnGrabAttemptTimedOut();
	void BeginConfirmedCapture();
	void CreateGrabCaptureSphere();
	void DestroyGrabCaptureSphere();
	void SetGrabCaptureSphereEnabled(bool bEnabled);
	void TryConfirmCaptureFromActor(AActor* OtherActor);
	void DrawGrabCaptureDebugSphere() const;
	void FaceTargetForGrab(APawn* AvatarPawn, const AActor* TargetActor) const;
	void ApplyGrabMovementTuning(APawn* AvatarPawn);
	void RestoreGrabMovementTuning();
	void StartCaptureTransfer();
	void ResetGrabAttemptState();

	AFTCaptureDestination* FindNearestCaptureDestination(const FVector& From) const;

	TWeakObjectPtr<AActor> PendingTarget;
	TWeakObjectPtr<UFTCaptureEscapeComponent> PendingEscapeComp;
	TWeakObjectPtr<AFTSecurityCharacter> PendingSecurity;
	TWeakObjectPtr<USceneComponent> PendingAttachPoint;
	FName PendingAttachSocketName = NAME_None;
	TWeakObjectPtr<AActor> CapturedTarget;
	TWeakObjectPtr<UFTCaptureEscapeComponent> TargetEscapeComp;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	TWeakObjectPtr<AAIController> CachedAIController;
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> GrabCaptureSphereComponent = nullptr;
	FTimerHandle FallbackTimerHandle;
	FTimerHandle CaptureDamageTimerHandle;
	FTimerHandle GrabAttemptTimeoutTimerHandle;
	// 지속 피해를 실제 경과시간으로 적용하기 위한 직전 틱의 월드 시간(타이머 간격이 밀려도 총량이 보존된다).
	float LastCaptureDamageTickTime = 0.0f;
	FAIRequestID CaptureTransferMoveRequestID = FAIRequestID::InvalidRequest;
	TWeakObjectPtr<UCharacterMovementComponent> TunedMovementComponent;
	float SavedMaxWalkSpeed = 0.0f;
	FRotator SavedRotationRate = FRotator::ZeroRotator;
	FDelegateHandle OwnerImmobilizedTagChangedHandle;
	bool bResolved = false;
	bool bBoundMoveCompleted = false;
	bool bWaitingForCaptureTransferMove = false;
	bool bGrabCaptureWindowOpen = false;
	bool bGrabCaptureConfirmed = false;
	bool bCapturedMessageBroadcast = false;
	bool bEscapedMessageBroadcast = false;
	bool bGrabMovementTuningApplied = false;
};
