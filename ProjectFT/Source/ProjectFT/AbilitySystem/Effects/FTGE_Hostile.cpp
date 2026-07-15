// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Hostile.h"

#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Hostile::UFTGE_Hostile()
{
	// 순수 마커: 즉시 적용 후 소멸(모디파이어/부여 태그 없음). 적용됐다는 사실만으로 대상의 감지 훅을 깨운다.
	// (Instant여도 OnGameplayEffectAppliedDelegateToSelf는 적용 말미에 무조건 발동하므로 대상이 이를 포착한다.)
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 공격성 식별 에셋 태그(Effect.Hostile). 대상은 GE 적용 시 이 태그로 "공격당함"을 판정한다.
	// 생성자에서는 CreateDefaultSubobject + GEComponents.Add (NewObject 기반 FindOrAddComponent는 크래시).
	UAssetTagsGameplayEffectComponent* AssetTagsComponent = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTagsComponent"));
	GEComponents.Add(AssetTagsComponent);
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(TAG_FT_Effect_Hostile);
	AssetTagsComponent->SetAndApplyAssetTagChanges(AssetTags);
}
