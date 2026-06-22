// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Poison.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

UFTGE_Poison::UFTGE_Poison()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
	Period = FScalableFloat(1.0f); // 1초마다 -10 (5초간 총 -50)

	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));
	Modifiers.Add(Mod);

	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
