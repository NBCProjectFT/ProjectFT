// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_BubbleTrap.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

UFTGE_BubbleTrap::UFTGE_BubbleTrap()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 자동 해제 타이머. 버블 투사체 DA의 EffectMagnitudes에서 Data.BubbleDuration으로 주입하고,
	// UFTGA_BubbleStackTrap이 살아 있는 BubbleStack GE의 SetByCaller 값을 TrapSpec에 전달한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_BubbleDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 상태는 '있다/없다'라 누적 금지: 대상당 1스택, 재적용 시 지속시간만 갱신. (모든 디버프 공통 정책.)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 활성인 동안 대상에게: 식별(Bubble) + 행동불능 우산(Immobilized, 이동정지+어빌리티차단) + GE 기반 탈출형(Escapable) 태그 부여.
	// 생성자에서는 CreateDefaultSubobject + GEComponents.Add (NewObject 기반 FindOrAddComponent는 크래시).
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Bubble);
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Immobilized);
	GrantedTags.Added.AddTag(TAG_FT_State_Debuff_Escapable);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);

	// 갇힘 '지속' 연출 GameplayCue. GE 수명과 함께 자동 발동/제거된다(비주얼은 GC_Bubble Notify가 담당).
	FGameplayEffectCue BubbleCue;
	BubbleCue.GameplayCueTags.AddTag(TAG_FT_GameplayCue_State_Bubble);
	GameplayCues.Add(BubbleCue);

	// 잡기 대칭 배타(A): 대상이 잡힌 상태(State.Captured)면 이 GE는 적용되지 않는다 → 잡힌 중엔 비눗방울에 안 걸림.
	// (반대 방향(잡기 시작 시 기존 자가CC 팝)은 UFTGA_Grab이 처리.) 새 자가CC GE(빙결/석화 등)도 같은 조건을 둘 것.
	UTargetTagRequirementsGameplayEffectComponent* RequirementsComponent = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("RequirementsComponent"));
	RequirementsComponent->ApplicationTagRequirements.IgnoreTags.AddTag(TAG_FT_State_Captured);
	GEComponents.Add(RequirementsComponent);

	// 버블에 갇힐 때 체력을 -1 감산 적용 (데미지 1을 주어 공격자 정보를 AI에게 전달 및 피격 이벤트 트리거)
	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = UFTAttributeSet::GetHealthAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.0f));
	Modifiers.Add(DamageMod);
}
