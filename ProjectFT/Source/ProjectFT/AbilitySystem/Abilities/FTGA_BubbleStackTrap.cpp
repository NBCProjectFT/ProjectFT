// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_BubbleStackTrap.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_BubbleTrap.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGA_BubbleStackTrap::UFTGA_BubbleStackTrap()
{
	// 스택 태그가 붙는 순간 자동 발동(누적 감시 시작). 스택이 모두 사라지면 자동 종료된다.
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_State_Debuff_BubbleStack;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(Trigger);

	// 이미 다른 행동불능(스턴 등)에 걸린 대상도 스택이 차면 갇혀야 하므로, 이 어빌리티는 Immobilized 차단에서 예외.
	ActivationBlockedTags.RemoveTag(TAG_FT_State_Debuff_Immobilized);

	TrapEffectClass = UFTGE_BubbleTrap::StaticClass();
}

float UFTGA_BubbleStackTrap::ResolveBubbleDurationFromActiveStacks(const UAbilitySystemComponent* ASC) const
{
	float ResolvedDuration = FMath::Max(DefaultBubbleDuration, KINDA_SMALL_NUMBER);
	float LatestStartTime = -TNumericLimits<float>::Max();

	if (!ASC)
	{
		return ResolvedDuration;
	}

	const FGameplayTagContainer StackTags(TAG_FT_State_Debuff_BubbleStack);
	const TArray<FActiveGameplayEffectHandle> StackHandles = ASC->GetActiveEffectsWithAllTags(StackTags);

	for (const FActiveGameplayEffectHandle& StackHandle : StackHandles)
	{
		const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(StackHandle);
		if (!ActiveEffect)
		{
			continue;
		}

		const float CandidateDuration = ActiveEffect->Spec.GetSetByCallerMagnitude(
			TAG_FT_Data_BubbleDuration,
			/*WarnIfNotFound=*/false,
			/*DefaultIfNotFound=*/-1.0f);

		if (CandidateDuration > 0.0f && ActiveEffect->StartWorldTime >= LatestStartTime)
		{
			ResolvedDuration = CandidateDuration;
			LatestStartTime = ActiveEffect->StartWorldTime;
		}
	}

	return ResolvedDuration;
}

void UFTGA_BubbleStackTrap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 활성 동안 스택 카운트를 감시한다(트리거는 카운트 1에서 발동하므로, 임계치까지 후속 변화를 지켜본다).
	// AnyCountChange여야 1→2→3… 매 카운트 변화를 받는다(NewOrRemoved는 있음/없음 전환에서만 발동해 임계 감지 불가).
	StackCountHandle = ASC->RegisterGameplayTagEvent(TAG_FT_State_Debuff_BubbleStack, EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UFTGA_BubbleStackTrap::OnStackCountChanged);

	UE_LOG(LogTemp, Warning, TEXT("[BubbleDebug] StackTrap ACTIVATED on %s. count=%d threshold=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), ASC->GetGameplayTagCount(TAG_FT_State_Debuff_BubbleStack), BubbleTrapThreshold);

	// 발동 시점에 이미 임계치를 넘겼을 수도 있으니 즉시 한 번 판정.
	TryTrap(ASC->GetGameplayTagCount(TAG_FT_State_Debuff_BubbleStack));
}

void UFTGA_BubbleStackTrap::OnStackCountChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	UE_LOG(LogTemp, Warning, TEXT("[BubbleDebug] StackCount -> %d (threshold %d)"), NewCount, BubbleTrapThreshold);
	TryTrap(NewCount);
}

void UFTGA_BubbleStackTrap::TryTrap(int32 CurrentStackCount)
{
	if (CurrentStackCount < BubbleTrapThreshold)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 이미 갇혀 있으면 재적용 금지.
	if (ASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Bubble))
	{
		return;
	}

	// 갇힘 GE 적용. 지속시간은 버블 스택 GE에 실려온 Data.BubbleDuration을 전달한다.
	if (TrapEffectClass)
	{
		FGameplayEffectSpecHandle TrapSpec = MakeOutgoingGameplayEffectSpec(TrapEffectClass);
		if (TrapSpec.IsValid())
		{
			const float BubbleDuration = ResolveBubbleDurationFromActiveStacks(ASC);
			TrapSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_BubbleDuration, BubbleDuration);
			ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, TrapSpec);
			UE_LOG(LogTemp, Warning, TEXT("[BubbleDebug] TRAP applied on %s (count=%d, GE duration=%.2f)"),
				*GetNameSafe(GetAvatarActorFromActorInfo()), CurrentStackCount, TrapSpec.Data->GetDuration());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BubbleDebug] TRAP skipped: TrapEffectClass is null."));
	}

	// 쌓인 스택 GE 제거 → 카운트 리셋(갇힘 해제 직후 남은 스택으로 즉시 재갇힘 방지).
	// 스택 태그가 사라지면 트리거 태그도 사라져 이 어빌리티는 자동 종료된다.
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_FT_State_Debuff_BubbleStack));
}

void UFTGA_BubbleStackTrap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (StackCountHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RegisterGameplayTagEvent(TAG_FT_State_Debuff_BubbleStack, EGameplayTagEventType::AnyCountChange).Remove(StackCountHandle);
		}
		StackCountHandle.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
