// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_UseItem.h"
#include "FTGA_UseHealPotion.generated.h"

/**
 * 회복약 사용 어빌리티(UFTGA_UseItem의 C++ 예시). 시전 후 체력을 회복하고 쿨다운을 건다.
 * 효과/시전/쿨다운 값만 지정하는 얇은 파생 — 새 아이템은 보통 이처럼 BP 또는 C++로 값만 채운다.
 */
UCLASS()
class PROJECTFT_API UFTGA_UseHealPotion : public UFTGA_UseItem
{
	GENERATED_BODY()

public:
	UFTGA_UseHealPotion();
};
