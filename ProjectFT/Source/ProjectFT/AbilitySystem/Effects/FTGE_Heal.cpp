// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Heal.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Heal::UFTGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 회복량은 고정값이 아니라 아이템 데이터가 SetByCaller(Data.Heal)로 주입한다(UFTGA_UseItem이 적용 시 주입).
	FSetByCallerFloat HealByCaller;
	HealByCaller.DataTag = TAG_FT_Data_Heal;

	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealByCaller);
	Modifiers.Add(Mod);
}
