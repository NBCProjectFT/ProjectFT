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
	// WaitGameplayEvent/몽타주 콜백에서 이번 공격 상태를 들고 있어야 하므로 인스턴스형으로 사용한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 모든 아이템 사용 입력은 Event.UseItem으로 들어오고, 실제 실행할 Ability는 ItemData.UseData.UseAbility가 결정한다.
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

	// PlayerCharacter/EquipmentComponent가 Payload.OptionalObject에 넣어준 ItemDataAsset을 읽는다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveMeleeData = Cast<UFTMeleeDataAsset>(ActiveItemData);

	// 한 번의 몽타주 공격에서 중복 타격을 막기 위한 상태를 초기화한다.
	HitActors.Reset();
	bMeleeTraceActive = false;
	bTraceActive = false;

	// 이 Ability는 UFTMeleeDataAsset 전용이다. 다른 타입이 들어오면 즉시 취소한다.
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

	// 비용/쿨다운 커밋은 실제 공격이 시작되기 직전에 처리한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndMeleeAbility(true);
		return;
	}

	// AnimNotifyState가 판정 구간 시작을 알려주는 이벤트를 기다린다.
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

	// AnimNotifyState가 Tick마다 찾은 타격 대상을 보내는 이벤트를 기다린다.
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

	// AnimNotifyState가 판정 구간 종료를 알려주는 이벤트를 기다린다.
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

	// 몽타주 재생 수명과 Ability 수명을 묶는다. 몽타주가 끝나거나 끊기면 Ability도 종료된다.
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
	// 새 판정 구간이 시작될 때 이전 구간의 타격 기록을 비운다.
	HitActors.Reset();
	bMeleeTraceActive = true;
}

void UFTGA_MeleeAction::HandleMeleeHitEvent(FGameplayEventData Payload)
{
	// NotifyState가 보낸 Hit 이벤트여도 Begin~End 구간 밖이면 무시한다.
	if (!bMeleeTraceActive)
	{
		return;
	}

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor)
	{
		return;
	}

	// 자기 자신은 근접 공격 대상에서 제외한다.
	if (HitActor == GetAvatarActorFromActorInfo())
	{
		return;
	}

	// 한 번의 공격 구간에서 같은 액터에게 여러 번 효과가 들어가지 않게 막는다.
	if (HitActors.Contains(HitActor))
	{
		return;
	}

	HitActors.Add(HitActor);

	FGameplayAbilityTargetDataHandle TargetData = Payload.TargetData;
	
	// Notify 쪽에서 TargetData를 만들지 못한 경우 Actor 기반 TargetData를 보강해서 GE 적용 경로를 통일한다.
	if (TargetData.Num() <= 0)
	{
		TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor);
	}

	// 실제 데미지/상태이상은 ItemData.UseData.UseEffects에 들어 있는 GameplayEffect 목록이 처리한다.
	ApplyUseEffects(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		&TargetData
	);

	ApplyDamageToDamageableTarget(HitActor, Payload.ContextHandle.GetHitResult());
}

void UFTGA_MeleeAction::HandleMeleeEndEvent(FGameplayEventData Payload)
{
	// 이후 늦게 들어오는 Hit 이벤트는 무시되도록 판정 상태를 닫는다.
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
	// 정상 종료/취소 모두 같은 정리 경로를 사용한다.
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
