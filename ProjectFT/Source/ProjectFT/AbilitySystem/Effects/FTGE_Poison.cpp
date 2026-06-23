// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Poison.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

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

	// 모든 버프/디버프는 대상당 1개(AggregateByTarget+limit1): 누적 없이 재적용 시 지속시간만 갱신(독도 스택 아닌 갱신).
	// StackingType은 5.7 deprecated라 직접 대입을 pragma로 감싼다(세터 SetStackingType은 WITH_EDITOR 전용).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 중독 지속 동안 대상에게 State.Debuff.Poison 부여 → UI/해독/면역이 GE 클래스를 몰라도 태그로 관찰·반응.
	// (생성자에서는 NewObject 기반 FindOrAddComponent 금지 — CreateDefaultSubobject + GEComponents.Add.)
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Poison);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
