#include "FTUseDataEffectLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Hostile.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

FGameplayTag UFTUseDataEffectLibrary::ResolveCooldownTag(const FTItemUseStruct& UseData)
{
	return UseData.CooldownTag.IsValid() ? UseData.CooldownTag : TAG_FT_Cooldown_ItemUse;
}

int32 UFTUseDataEffectLibrary::ApplyUseEffectsFromAbility(
	UFTGameplayAbility* Ability,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FTItemUseStruct& UseData,
	const FGameplayAbilityTargetDataHandle* TargetData)
{
	if (!Ability || !ActorInfo)
	{
		return 0;
	}

	int32 AppliedCount = 0;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : UseData.UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		// 공격 표식(UFTGE_Hostile)은 '실제 효과'가 아니다 — 적용은 하되(대상 감지용) AppliedCount엔 세지 않는다.
		// 표식/추가 no-op GE 때문에 AppliedCount<=0 실패 판정 호출부(FTUseDataEffectComponent 등)가 오판하지 않도록.
		const bool bIsHostileMarker = EffectClass->IsChildOf(UFTGE_Hostile::StaticClass());

		const FGameplayEffectSpecHandle EffectSpec = Ability->MakeOutgoingGameplayEffectSpec(
			Handle,
			ActorInfo,
			ActivationInfo,
			EffectClass,
			Ability->GetAbilityLevel(Handle, ActorInfo));
		if (!EffectSpec.IsValid())
		{
			continue;
		}

		ApplySetByCallerMagnitudes(EffectSpec, UseData);

		if (TargetData)
		{
			const TArray<FActiveGameplayEffectHandle> Handles = Ability->ApplyGameplayEffectSpecToTargetPublic(
				Handle,
				ActorInfo,
				ActivationInfo,
				EffectSpec,
				*TargetData);
			if (!bIsHostileMarker)
			{
				AppliedCount += Handles.Num();
			}
		}
		else
		{
			const FActiveGameplayEffectHandle AppliedHandle = Ability->ApplyGameplayEffectSpecToOwnerPublic(
				Handle,
				ActorInfo,
				ActivationInfo,
				EffectSpec);
			if (AppliedHandle.IsValid() && !bIsHostileMarker)
			{
				++AppliedCount;
			}
		}
	}

	return AppliedCount;
}

int32 UFTUseDataEffectLibrary::ApplyUseEffectsFromASC(
	UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC,
	const FTItemUseStruct& UseData,
	const float Level)
{
	if (!TargetASC)
	{
		return 0;
	}

	UAbilitySystemComponent* SpecSourceASC = SourceASC ? SourceASC : TargetASC;
	if (!SpecSourceASC)
	{
		return 0;
	}

	FGameplayEffectContextHandle EffectContext = SpecSourceASC->MakeEffectContext();
	if (AActor* SourceAvatar = SpecSourceASC->GetAvatarActor())
	{
		EffectContext.AddSourceObject(SourceAvatar);
	}

	int32 AppliedCount = 0;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : UseData.UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const bool bIsHostileMarker = EffectClass->IsChildOf(UFTGE_Hostile::StaticClass());

		const FGameplayEffectSpecHandle EffectSpec = SpecSourceASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
		if (!EffectSpec.IsValid())
		{
			continue;
		}

		ApplySetByCallerMagnitudes(EffectSpec, UseData);

		const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (AppliedHandle.IsValid() && !bIsHostileMarker)
		{
			++AppliedCount;
		}
	}

	return AppliedCount;
}

int32 UFTUseDataEffectLibrary::ApplyUseEffectsToActor(
	AActor* SourceActor,
	AActor* TargetActor,
	const FTItemUseStruct& UseData,
	const float Level)
{
	if (!TargetActor)
	{
		return 0;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return 0;
	}

	UAbilitySystemComponent* SourceASC = SourceActor
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor)
		: nullptr;
	return ApplyUseEffectsFromASC(SourceASC, TargetASC, UseData, Level);
}

bool UFTUseDataEffectLibrary::IsUseDataOnCooldown(UAbilitySystemComponent* ASC, const FTItemUseStruct& UseData)
{
	return ASC
		&& UseData.CooldownSeconds > 0.0f
		&& ASC->HasMatchingGameplayTag(ResolveCooldownTag(UseData));
}

bool UFTUseDataEffectLibrary::ApplyCooldownFromASC(
	UAbilitySystemComponent* SourceASC,
	const FTItemUseStruct& UseData,
	TSubclassOf<UGameplayEffect> CooldownEffectClass,
	const float Level)
{
	if (!SourceASC || UseData.CooldownSeconds <= 0.0f)
	{
		return false;
	}

	if (!CooldownEffectClass)
	{
		CooldownEffectClass = UFTGE_Cooldown::StaticClass();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	if (AActor* SourceAvatar = SourceASC->GetAvatarActor())
	{
		EffectContext.AddSourceObject(SourceAvatar);
	}

	const FGameplayEffectSpecHandle CooldownSpec = SourceASC->MakeOutgoingSpec(CooldownEffectClass, Level, EffectContext);
	if (!CooldownSpec.IsValid())
	{
		return false;
	}

	CooldownSpec.Data->DynamicGrantedTags.AddTag(ResolveCooldownTag(UseData));
	CooldownSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Cooldown, UseData.CooldownSeconds);
	return SourceASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get()).IsValid();
}

void UFTUseDataEffectLibrary::ApplySetByCallerMagnitudes(
	const FGameplayEffectSpecHandle EffectSpec,
	const FTItemUseStruct& UseData)
{
	if (!EffectSpec.IsValid())
	{
		return;
	}

	for (const TPair<FGameplayTag, float>& Magnitude : UseData.EffectMagnitudes)
	{
		if (Magnitude.Key.IsValid())
		{
			EffectSpec.Data->SetSetByCallerMagnitude(Magnitude.Key, Magnitude.Value);
		}
	}
}
