// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Cooldown.generated.h"

/**
 * 아이템 사용 어빌리티 공용 쿨다운 GameplayEffect(Duration).
 * 지속시간은 고정하지 않고 어빌리티가 SetByCaller(Data.Cooldown)로 주입한다.
 * 적용 동안 소유자에게 Cooldown.ItemUse 태그를 부여하여 어빌리티의 CheckCooldown이 재사용을 차단하도록 한다.
 */
UCLASS()
class PROJECTFT_API UFTGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Cooldown();
};
