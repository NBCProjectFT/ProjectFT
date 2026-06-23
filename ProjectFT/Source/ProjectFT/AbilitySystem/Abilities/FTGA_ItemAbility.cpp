// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_ItemAbility.h"

#include "GameplayEffect.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

UFTGA_ItemAbility::UFTGA_ItemAbility()
{
	// 쿨다운은 공용 쿨다운 GE로 처리(지속시간은 ApplyCooldown에서 ActiveUseData.CooldownSeconds로 주입).
	CooldownGameplayEffectClass = UFTGE_Cooldown::StaticClass();
}

const UFTItemDataAsset* UFTGA_ItemAbility::CacheActiveItem(const FGameplayEventData* TriggerEventData)
{
	const UFTItemDataAsset* ItemAsset = TriggerEventData ? Cast<UFTItemDataAsset>(TriggerEventData->OptionalObject) : nullptr;
	if (ItemAsset)
	{
		ActiveUseData = ItemAsset->ItemData.UseData;
	}
	return ItemAsset;
}

void UFTGA_ItemAbility::ApplyUseEffects(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayAbilityTargetDataHandle* TargetData)
{
	// UseEffects를 순서대로 적용한다. EffectMagnitudes는 SetByCaller로 각 스펙에 주입(각 GE는 필요한 태그만 사용).
	for (const TSubclassOf<UGameplayEffect>& EffectClass : ActiveUseData.UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}
		const FGameplayEffectSpecHandle EffectSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, EffectClass, GetAbilityLevel(Handle, ActorInfo));
		if (!EffectSpec.IsValid())
		{
			continue;
		}
		for (const TPair<FGameplayTag, float>& Magnitude : ActiveUseData.EffectMagnitudes)
		{
			EffectSpec.Data->SetSetByCallerMagnitude(Magnitude.Key, Magnitude.Value);
		}

		if (TargetData)
		{
			// 트레이스/투사체 등으로 맞힌 대상에게 적용한다(ASC 없는 대상이면 자동 무시).
			ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, EffectSpec, *TargetData);
		}
		else
		{
			// 자신(Owner)에게 적용한다.
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, EffectSpec);
		}
	}
}

void UFTGA_ItemAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ActiveUseData.CooldownSeconds <= 0.0f)
	{
		return; // 쿨다운 없음.
	}

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	// 공용 쿨다운 GE에 이 아이템의 쿨다운 시간을 SetByCaller로 주입해 적용한다.
	const FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (CooldownSpec.IsValid())
	{
		// 아이템별 쿨다운 태그를 동적으로 부여(없으면 공용 폴백) → 호출측이 이 태그로 재사용을 차단한다.
		CooldownSpec.Data->DynamicGrantedTags.AddTag(ResolveCooldownTag(ActiveUseData));
		CooldownSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Cooldown, ActiveUseData.CooldownSeconds);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
	}
}

void UFTGA_ItemAbility::OnItemConsumed()
{
	// 기본 구현 없음. 인벤토리 차감/사용 연출은 후속 작업에서 추가한다.
}

FGameplayTag UFTGA_ItemAbility::ResolveCooldownTag(const FTItemUseStruct& UseData)
{
	return UseData.CooldownTag.IsValid() ? UseData.CooldownTag : TAG_FT_Cooldown_ItemUse;
}
