// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Slow.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Slow::UFTGE_Slow()
{
	// 지속 정책 + 지속시간은 아이템 데이터가 SetByCaller(Data.Duration)로 주입한다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 이동속도 ×0.5 (Multiplicitive = 곱연산).
	FGameplayModifierInfo Mod;
	Mod.Attribute = UFTAttributeSet::GetMoveSpeedAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	Modifiers.Add(Mod);

	// 모든 버프/디버프는 대상당 1개(AggregateByTarget+limit1): 누적 없이 재적용 시 지속시간만 갱신.
	// StackingType은 5.7 deprecated라 직접 대입을 pragma로 감싼다(세터 SetStackingType은 WITH_EDITOR 전용).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 슬로우 지속 동안 대상에게 State.Debuff.Slow 부여(UI/해독/조건분기가 GE 클래스를 몰라도 태그로 관찰).
	// 생성자에서는 NewObject 기반 FindOrAddComponent 금지 → CreateDefaultSubobject + GEComponents.Add.
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Slow);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
