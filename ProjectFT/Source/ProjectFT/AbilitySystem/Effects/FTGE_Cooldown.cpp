// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGE_Cooldown.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGE_Cooldown::UFTGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속시간은 고정값이 아니라 어빌리티가 CooldownSeconds를 SetByCaller(Data.Cooldown)로 주입한다.
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = TAG_FT_Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// 부여 태그는 의도적으로 '정적'으로 두지 않는다 — 아이템별 독립 쿨다운을 위해서다.
	// 어빌리티(UFTGA_ItemAbility::ApplyCooldown)가 아이템별 태그를 DynamicGrantedTags로 얹어 적용하고,
	// 차단은 호출측(AFTPlayerCharacter)이 그 태그로 개별 판정한다. 정적 부여 태그를 두면 모든 쿨다운에 공통으로
	// 붙어 표준 CheckCooldown이 '아이템 공유 차단'을 걸어 독립 쿨다운이 깨진다(그래서 표준 CooldownGameplayEffectClass
	// 경로도 쓰지 않고 어빌리티가 직접 Apply한다 → 이 GE는 정적 태그 검증 대상이 아니다).
}
