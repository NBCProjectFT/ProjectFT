// Fill out your copyright notice in the Description page of Project Settings.

#include "FTAttributeSet.h"

#include "GameplayEffectExtension.h"

UFTAttributeSet::UFTAttributeSet()
{
	// 스탯 기본값 초기화(이전 핸드롤 스탯의 기본값과 동일).
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitMoveSpeed(600.0f);
	InitDexterity(1.0f);
	InitSprintSpeedMultiplier(1.5f);
	InitCrouchSpeedMultiplier(0.5f);
}

void UFTAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute()
		|| Attribute == GetMoveSpeedAttribute() || Attribute == GetDexterityAttribute()
		|| Attribute == GetSprintSpeedMultiplierAttribute() || Attribute == GetCrouchSpeedMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UFTAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;

	if (Attr == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		if (GetHealth() <= 0.0f)
		{
			OnOutOfHealth.Broadcast();
		}
	}
	else if (Attr == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Attr == GetMaxHealthAttribute())
	{
		// 최대 체력이 줄면 현재값을 끌어내린다.
		SetHealth(FMath::Min(GetHealth(), GetMaxHealth()));
	}
	else if (Attr == GetMaxStaminaAttribute())
	{
		SetStamina(FMath::Min(GetStamina(), GetMaxStamina()));
	}
}
