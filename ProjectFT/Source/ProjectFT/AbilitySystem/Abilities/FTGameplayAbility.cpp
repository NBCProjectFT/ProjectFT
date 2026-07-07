// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGameplayAbility.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGameplayAbility::UFTGameplayAbility()
{
	// 시전 대기(AbilityTask) 등 인스턴스 상태가 필요하므로 액터당 인스턴스화한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 행동불능(스턴/마비/비눗방울 등) 상태에선 모든 FT 어빌리티 활성을 차단한다. 개별 효과가 아니라 우산 태그 하나로 판정하므로
	// 새 행동불능 효과가 추가돼도 이 코드는 그대로다(예외가 필요한 어빌리티는 파생에서 이 태그를 제거).
	ActivationBlockedTags.AddTag(TAG_FT_State_Debuff_Immobilized);
}

FGameplayTag UFTGameplayAbility::GetTriggerEventTag() const
{
	// 첫 번째 GameplayEvent 트리거의 태그를 돌려준다(아이템 사용 어빌리티는 이 태그로 발동된다).
	for (const FAbilityTriggerData& Trigger : AbilityTriggers)
	{
		if (Trigger.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent)
		{
			return Trigger.TriggerTag;
		}
	}
	return FGameplayTag();
}

FActiveGameplayEffectHandle UFTGameplayAbility::ApplyGameplayEffectSpecToOwnerPublic(
	const FGameplayAbilitySpecHandle AbilityHandle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEffectSpecHandle SpecHandle) const
{
	return ApplyGameplayEffectSpecToOwner(AbilityHandle, ActorInfo, ActivationInfo, SpecHandle);
}

TArray<FActiveGameplayEffectHandle> UFTGameplayAbility::ApplyGameplayEffectSpecToTargetPublic(
	const FGameplayAbilitySpecHandle AbilityHandle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEffectSpecHandle SpecHandle,
	const FGameplayAbilityTargetDataHandle& TargetData) const
{
	return ApplyGameplayEffectSpecToTarget(AbilityHandle, ActorInfo, ActivationInfo, SpecHandle, TargetData);
}
