// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerAttributeSet.h"

#include "GameplayEffectExtension.h"

UFTPlayerAttributeSet::UFTPlayerAttributeSet()
{
	// 플레이어 전용 스탯 기본값(공용 세트에서 분리하며 기존 기본값 그대로 이전).
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitDexterity(1.0f);
	InitSprintSpeedMultiplier(1.5f);
	InitCrouchSpeedMultiplier(0.5f);
}

void UFTPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute() || Attribute == GetDexterityAttribute()
		|| Attribute == GetSprintSpeedMultiplierAttribute() || Attribute == GetCrouchSpeedMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UFTPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;

	if (Attr == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Attr == GetMaxStaminaAttribute())
	{
		// 최대 스태미나가 줄면 현재값을 끌어내린다.
		SetStamina(FMath::Min(GetStamina(), GetMaxStamina()));
	}
}
