// Fill out your copyright notice in the Description page of Project Settings.

#include "FTStateTreeTask_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
	// 액터의 ASC가 해당 태그를 보유 중인지(붙잡기 진행 감시용). ASC 없거나 태그 비면 false.
	bool ActorHasGameplayTag(AActor* Actor, const FGameplayTag& Tag)
	{
		if (!Actor || !Tag.IsValid())
		{
			return false;
		}
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		return ASC && ASC->HasMatchingGameplayTag(Tag);
	}
}

EStateTreeRunStatus FFTStateTreeTask_SendGameplayEvent::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.EventOwner || !InstanceData.EventTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// EventOwner의 ASC로 GameplayEvent 발송(페이로드 Target = 잡을 대상). 어빌리티의 GameplayEvent 트리거가 발동한다.
	FGameplayEventData Payload;
	Payload.EventTag = InstanceData.EventTag;
	Payload.Instigator = InstanceData.EventOwner;
	Payload.Target = InstanceData.PayloadTarget;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InstanceData.EventOwner, InstanceData.EventTag, Payload);

	// 진행 감시 태그가 없으면 발사 후 즉시 완료.
	if (!InstanceData.RunningWhileTag.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 발동된 어빌리티가 진행 태그(예: State.Grabbing)를 부여했으면, 그 태그가 사라질 때까지 상태 유지.
	return ActorHasGameplayTag(InstanceData.EventOwner, InstanceData.RunningWhileTag)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FFTStateTreeTask_SendGameplayEvent::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 진행 태그가 사라지면(어빌리티 종료) 완료. (감시 태그가 없으면 EnterState에서 이미 Succeeded로 끝남.)
	return ActorHasGameplayTag(InstanceData.EventOwner, InstanceData.RunningWhileTag)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Succeeded;
}
