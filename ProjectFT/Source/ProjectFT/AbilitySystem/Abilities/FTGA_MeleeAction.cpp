// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_MeleeAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"

UFTGA_MeleeAction::UFTGA_MeleeAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_MeleeAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveMeleeData = Cast<UFTMeleeDataAsset>(ActiveItemData);

	HitActors.Reset();
	bMeleeTraceActive = false;
	bTraceActive = false;

	if (!ActiveItemData || !ActiveMeleeData)
	{
		EndMeleeAbility(true);
		return;
	}

	const FFTMeleeActionStruct* MeleeData = GetMeleeActionData();

	if (!MeleeData)
	{
		EndMeleeAbility(true);
		return;
	}

	if (!MeleeData->AttackMontage)
	{
		EndMeleeAbility(true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndMeleeAbility(true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* BeginEventTask =
	UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		TAG_FT_Event_Melee_Begin,
		nullptr,
		false,
		true
	);

	if (BeginEventTask)
	{
		BeginEventTask->EventReceived.AddDynamic(
			this,
			&UFTGA_MeleeAction::HandleMeleeBeginEvent
		);
		BeginEventTask->ReadyForActivation();
	}

	UAbilityTask_WaitGameplayEvent* HitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_FT_Event_Melee_Hit,
			nullptr,
			false,
			true
		);

	if (HitEventTask)
	{
		HitEventTask->EventReceived.AddDynamic(
			this,
			&UFTGA_MeleeAction::HandleMeleeHitEvent
		);
		HitEventTask->ReadyForActivation();
	}

	UAbilityTask_WaitGameplayEvent* EndEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_FT_Event_Melee_End,
			nullptr,
			false,
			true
		);

	if (EndEventTask)
	{
		EndEventTask->EventReceived.AddDynamic(
			this,
			&UFTGA_MeleeAction::HandleMeleeEndEvent
		);
		EndEventTask->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("MeleeActionMontage"),
		MeleeData->AttackMontage,
		1.0f
		);

	if (!MontageTask)
	{
		EndMeleeAbility(true);
		return;
	}
	
	MontageTask->OnCompleted.AddDynamic(this, &UFTGA_MeleeAction::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_MeleeAction::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UFTGA_MeleeAction::HandleMontageInterrupted);

	MontageTask->ReadyForActivation();
}

void UFTGA_MeleeAction::HandleMeleeBeginEvent(FGameplayEventData Payload)
{
	HitActors.Reset();
	bMeleeTraceActive = true;
}

void UFTGA_MeleeAction::HandleMeleeHitEvent(FGameplayEventData Payload)
{
	
	if (!bMeleeTraceActive)
	{
		return;
	}

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor)
	{
		return;
	}

	if (HitActor == GetAvatarActorFromActorInfo())
	{
		return;
	}

	if (HitActors.Contains(HitActor))
	{
		return;
	}

	HitActors.Add(HitActor);

	FGameplayAbilityTargetDataHandle TargetData = Payload.TargetData;
	
	if (TargetData.Num() <= 0)
	{
		TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor);
	}

	ApplyUseEffects(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		&TargetData
	);
}

void UFTGA_MeleeAction::HandleMeleeEndEvent(FGameplayEventData Payload)
{
	bMeleeTraceActive = false;
}


void UFTGA_MeleeAction::HandleMontageCompleted()
{
	EndMeleeAbility(false);
}

void UFTGA_MeleeAction::HandleMontageInterrupted()
{
	EndMeleeAbility(true);
}

void UFTGA_MeleeAction::EndMeleeAbility(bool bWasCancelled)
{
	
	bMeleeTraceActive = false;
	bTraceActive = false;
	HitActors.Reset();

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled
	);
}

const FFTMeleeActionStruct* UFTGA_MeleeAction::GetMeleeActionData() const
{
	return ActiveMeleeData ? &ActiveMeleeData->MeleeActionData : nullptr;
}
