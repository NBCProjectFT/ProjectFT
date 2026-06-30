// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_UseHealingItem.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

UFTGA_UseHealingItem::UFTGA_UseHealingItem()
{}

void UFTGA_UseHealingItem::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 페이로드로부터 아이템 데이터를 먼저 캐싱 및 검증합니다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	if (!ItemAsset)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 소유자의 현재 체력을 검사하여 가득 차 있다면 시전을 취소시킵니다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const UFTAttributeSet* AttributeSet = ActorInfo->AbilitySystemComponent->GetSet<UFTAttributeSet>();
		if (AttributeSet)
		{
			const float CurrentHealth = AttributeSet->GetHealth();
			const float MaxHealth = AttributeSet->GetMaxHealth();

			if (CurrentHealth >= MaxHealth)
			{
				UE_LOG(LogTemp, Log, TEXT("회복 어빌리티 취소: 현재 체력이 이미 최대치(%f/%f)입니다."), CurrentHealth, MaxHealth);
				
				// 비용 차감 및 쿨다운 없이 안전하게 취소 처리
				EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
				return;
			}
		}
	}

	// 체력 검사를 통과했다면 부모(UFTGA_UseItem)의 시전(CastTime) 대기 및 효과 적용 로직을 실행합니다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
