// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "FTGameplayAbility.h"
#include "TimerManager.h"
#include "ProjectFT/Struct/FTStruggleGaugeStruct.h"
#include "FTGA_EscapableDebuff.generated.h"

/**
 * '연타로 탈출하는' 자가 상태이상(비눗방울/빙결 등)의 공용 탈출 어빌리티.
 *
 * 트리거: State.Debuff.Escapable이 붙는 순간(OwnedTagPresent) 자동 발동. 활성 동안 Event.Struggle(플레이어 발버둥)을 받아
 * 공용 게이지를 채우고, 가득 차면 현재 추적 중인 State.Debuff.Escapable GE를 제거해 상태를 해제한다.
 * 자동 해제(타이머)는 그 GE의 Duration이 담당하며, 만료되면 State.Debuff.Escapable이 사라져 이 어빌리티도 자동 종료된다.
 * 새 탈출형 상태가 들어오면 기존 탈출형 GE는 즉시 제거하고 새 GE 기준으로 게이지를 다시 시작한다.
 *
 * 효과마다 새 클래스를 만들 필요 없이, 이 하나가 데이터(제거 대상 = Debuff.Escapable을 부여한 GE)로 모든 GE 기반 탈출형을 처리한다.
 * 잡기(두-바디)는 자체 컴포넌트가 처리하므로, 잡힌 중(State.Captured)에는 이 어빌리티가 활성화되지 않는다.
 */
UCLASS()
class PROJECTFT_API UFTGA_EscapableDebuff : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_EscapableDebuff();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	float GetEscapeProgress() const { return Gauge.GetProgress(); }

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	float GetAccumulatedStruggle() const { return Gauge.GetAccumulated(); }

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	float GetEscapeThreshold() const { return Gauge.GetThreshold(); }

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	float GetRemainingEscapeTime() const;

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	float GetTotalEscapeTime() const { return AutoEscapeDurationSeconds; }

	UFUNCTION(BlueprintPure, Category = "FT|Escape")
	static bool GetActiveEscapableDebuffInfo(AActor* TargetActor, float& OutProgress, float& OutRemainingTime, float& OutTotalTime, float& OutAccumulatedStruggle, float& OutEscapeThreshold);

protected:
	// 탈출에 필요한 총 struggle 양. 자동 해제 시간 동안 자연 증가로 이 값까지 차고, 좌우 연타가 추가로 가속한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.0"))
	float EscapeThreshold = 12.0f;

	// 자동 해제 시간 정보를 못 읽었을 때 쓰는 fallback. 정상 버블은 UFTGE_BubbleTrap Duration을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.01"))
	float FallbackAutoEscapeSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.01"))
	float EscapeTickInterval = 0.05f;

	// Event.Struggle 1회가 자동 탈출 시간을 몇 초 앞당기는지. 기본 0.2초.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.0"))
	float SecondsReducedPerStruggleInput = 0.2f;

	// 연타 탈출 게이지. PassiveGainPerSecond는 활성 시점의 Debuff.Escapable GE Duration에 맞춰 계산된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape")
	FTStruggleGaugeStruct Gauge;

private:
	// Event.Struggle(발버둥 1회) 수신 → 게이지 상승. 가득 차면 Debuff.Escapable을 부여한 GE 제거 후 종료.
	UFUNCTION()
	void OnStruggleEvent(FGameplayEventData Payload);

	void OnEscapableTagCountChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void TickEscapeGauge();
	void TryCompleteEscape();
	void RemoveEscapableEffects();
	void StartEscapeTick();
	void InitializeEscapeGauge(const UAbilitySystemComponent* ASC);
	FActiveGameplayEffectHandle KeepNewestEscapableEffect(UAbilitySystemComponent* ASC) const;
	float ResolveAutoEscapeDurationSeconds(const UAbilitySystemComponent* ASC) const;

	static UFTGA_EscapableDebuff* FindActiveEscapableDebuffAbility(AActor* TargetActor);

	FTimerHandle EscapeTickTimerHandle;
	FDelegateHandle EscapableTagCountHandle;
	FActiveGameplayEffectHandle TrackedEscapableEffectHandle;
	float AutoEscapeDurationSeconds = 0.0f;
	float PassiveEscapeGainPerSecond = 0.0f;
	float LastEscapeTickTime = 0.0f;
};
