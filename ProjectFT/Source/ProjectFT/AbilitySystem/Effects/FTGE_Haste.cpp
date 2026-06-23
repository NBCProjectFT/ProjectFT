// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Haste.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Haste::UFTGE_Haste()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

	// 이동속도 ×1.5 (GAS Multiplicitive = 곱연산).
	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetMoveSpeedAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));
	Modifiers.Add(Mod);

	// 모든 버프/디버프는 대상당 1개(AggregateByTarget+limit1): 누적 없이 재적용 시 지속시간만 갱신.
	// StackingType은 5.7 deprecated라 직접 대입을 pragma로 감싼다(세터 SetStackingType은 WITH_EDITOR 전용).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 버프 지속 동안 대상에게 State.Buff.Haste 부여(UI/디스펠/조건 분기가 GE 클래스를 몰라도 태그로 관찰).
	// 생성자에서는 NewObject 기반 FindOrAddComponent 금지 → CreateDefaultSubobject + GEComponents.Add.
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Buff_Haste);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
