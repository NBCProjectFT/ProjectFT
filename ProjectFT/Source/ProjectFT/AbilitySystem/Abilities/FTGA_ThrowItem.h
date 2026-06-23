// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "FTGA_ThrowItem.generated.h"

/**
 * [전용 로직 어빌리티] 들고 있는 아이템을 시점 정면으로 던진다.
 *  - "무엇을 적용하나"(적중 시 효과) = 데이터(UseData.UseEffects)  ← 베이스(ApplyUseEffects)가 처리
 *  - "어떻게 던지나"(투사체 스폰 + 발사) = 이 GA만의 고유 로직     ← GE로 표현 불가라 전용 GA
 */
UCLASS()
class PROJECTFT_API UFTGA_ThrowItem : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_ThrowItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 던지는 초기 속도(cm/s). BP로 파생해 아이템 종류별로 조정 가능. (테이저의 TraceRange와 같은 패턴)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Throw", meta = (ClampMin = "0.0"))
	float ThrowSpeed = 1200.0f;
};
