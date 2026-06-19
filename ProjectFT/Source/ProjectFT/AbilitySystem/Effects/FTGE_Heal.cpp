// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Heal.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

UFTGE_Heal::UFTGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(50.0f));
	Modifiers.Add(Mod);
}
