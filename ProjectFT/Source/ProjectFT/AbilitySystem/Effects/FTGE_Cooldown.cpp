// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Cooldown.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Cooldown::UFTGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속시간은 고정값이 아니라 어빌리티가 CooldownSeconds를 SetByCaller로 주입한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 적용 동안 소유자에게 쿨다운 태그를 부여 → 어빌리티 CheckCooldown이 재사용을 차단.
	// 생성자에서는 이름 없는 NewObject를 쓰는 FindOrAddComponent/AddComponent 대신
	// CreateDefaultSubobject로 컴포넌트를 만들어 GEComponents에 직접 추가해야 한다.
	// (생성자 안에서의 NewObject(NAME_None) 호출은 "NewObject with empty name..." 치명적 에러를 유발한다.)
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer CooldownTags;
	CooldownTags.Added.AddTag(TAG_FT_Cooldown_ItemUse);
	TargetTagsComponent->SetAndApplyTargetTagChanges(CooldownTags);
}
