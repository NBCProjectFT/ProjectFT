// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_SpeedBuff.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

UFTGE_SpeedBuff::UFTGE_SpeedBuff()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

	// 이동속도 ×1.5. (GAS Multiplicitive는 곱연산 스택 — 우리 핸드롤의 가산 퍼센트와는 미세하게 다름. 주석 참고.)
	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetMoveSpeedAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));
	Modifiers.Add(Mod);

	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
