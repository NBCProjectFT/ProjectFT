// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "FTGA_Taser.generated.h"

/**
 * [예시: 고유 로직 어빌리티] 테이저 건. 카메라/시야 정면으로 라인트레이스해 처음 적중한 대상에게
 * 아이템 데이터(UseData)의 효과(예: UFTGE_Stun)를 적용한다.
 *  - "무엇을 적용하나" = 데이터(UseEffects / EffectMagnitudes)  ← 베이스(ApplyUseEffects)가 처리
 *  - "어떻게 맞히나"(트레이스 + 타깃 적용) = 이 GA만의 고유 로직  ← GE만으로 표현 불가라 전용 GA
 */
UCLASS()
class PROJECTFT_API UFTGA_Taser : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_Taser();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 정면 라인트레이스 사거리(cm).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Taser", meta = (ClampMin = "0.0"))
	float TraceRange = 1500.0f;
};
