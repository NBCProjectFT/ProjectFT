// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_BubbleStack.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_BubbleStack::UFTGE_BubbleStack()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 스택 유지 창. 이 창 안에 임계 스택수만큼 몰아 맞으면 갇힌다.
	// 아이템/투사체 DA의 EffectMagnitudes에서 Data.Duration으로 주입한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 스태킹을 두지 않는다: 적용마다 독립 인스턴스 → 태그 카운트가 곧 "동시에 살아있는 스택 수"가 된다.
	// 활성 동안 대상에게 State.Debuff.BubbleStack 부여(카운트 +1). 갇힘 판정은 UFTGA_BubbleStackTrap이 이 카운트로 한다.
	// 생성자에서는 CreateDefaultSubobject + GEComponents.Add (NewObject 기반 FindOrAddComponent는 크래시).
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_BubbleStack);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);

	// 이미 갇힌(State.Debuff.Bubble) 또는 잡힌(State.Captured) 대상에겐 스택을 쌓지 않는다.
	// 안 그러면 갇힌 대상을 계속 쏠 때 스택이 누적돼, 트랩이 풀리는 순간 즉시 재갇힘돼서 '영구 정지'처럼 보인다.
	UTargetTagRequirementsGameplayEffectComponent* RequirementsComponent = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("RequirementsComponent"));
	RequirementsComponent->ApplicationTagRequirements.IgnoreTags.AddTag(TAG_FT_State_Debuff_Bubble);
	RequirementsComponent->ApplicationTagRequirements.IgnoreTags.AddTag(TAG_FT_State_Captured);
	GEComponents.Add(RequirementsComponent);
}
