// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Cooldown.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Cooldown::UFTGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속시간은 고정값이 아니라 어빌리티가 CooldownSeconds를 SetByCaller로 주입한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 부여 태그(쿨다운 식별)는 고정하지 않는다 — 아이템별 분리를 위해 어빌리티(UFTGA_ItemAbility::ApplyCooldown)가
	// 스펙의 DynamicGrantedTags로 동적 주입한다(태그 미지정 아이템은 공용 Cooldown.ItemUse 폴백).
}
