// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Stun.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Stun::UFTGE_Stun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속시간은 고정값이 아니라 아이템 데이터가 SetByCaller(Data.StunDuration)로 주입한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_StunDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 스턴은 '상태(있다/없다)'라 누적되면 안 된다: 출처와 무관하게 대상당 1스택, 재적용 시 지속시간만 갱신.
	// (모든 버프/디버프 동일 정책 — 대상당 1개. StackingType은 5.7 deprecated라 pragma로 억제.)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 활성(스턴) 동안 대상에게 State.Debuff.Stun 부여.
	// 생성자에서는 NewObject 기반 FindOrAddComponent 금지 → CreateDefaultSubobject + GEComponents.Add.
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	// 식별 태그(State.Debuff.Stun) + 행동불능 우산 태그(State.Debuff.Immobilized)를 함께 부여한다.
	// 우산 태그로 캐릭터가 이동을 정지하고 어빌리티가 차단된다(개별 효과를 몰라도 됨).
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Stun);
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Immobilized);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
