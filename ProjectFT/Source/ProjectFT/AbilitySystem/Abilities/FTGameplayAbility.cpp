// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGameplayAbility.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGameplayAbility::UFTGameplayAbility()
{
	// 시전 대기(AbilityTask) 등 인스턴스 상태가 필요하므로 액터당 인스턴스화한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 스턴 등 행동불능 상태에선 모든 FT 어빌리티 활성을 차단한다(예외가 필요한 어빌리티는 파생에서 이 태그를 제거).
	ActivationBlockedTags.AddTag(TAG_FT_State_Debuff_Stun);
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
