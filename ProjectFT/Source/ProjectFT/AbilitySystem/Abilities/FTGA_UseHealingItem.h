// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_UseItem.h"
#include "FTGA_UseHealingItem.generated.h"

/**
 * @brief 회복 아이템 전용 사용 어빌리티.
 * 소유자(플레이어)의 체력이 이미 가득 차 있을 경우 사용 시전을 강제로 차단 및 취소합니다.
 */
UCLASS()
class PROJECTFT_API UFTGA_UseHealingItem : public UFTGA_UseItem
{
	GENERATED_BODY()

public:
	UFTGA_UseHealingItem();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
