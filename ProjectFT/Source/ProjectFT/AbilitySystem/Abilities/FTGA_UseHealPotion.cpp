// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_UseHealPotion.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Heal.h"

UFTGA_UseHealPotion::UFTGA_UseHealPotion()
{
	// 회복약: 1초 동안 들이켠 뒤 체력 회복(UFTGE_Heal), 5초 쿨다운.
	ItemEffect = UFTGE_Heal::StaticClass();
	CastTimeSeconds = 1.0f;
	CooldownSeconds = 5.0f;
}
