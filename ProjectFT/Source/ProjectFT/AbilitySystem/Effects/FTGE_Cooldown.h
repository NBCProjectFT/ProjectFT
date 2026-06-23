// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Cooldown.generated.h"

/**
 * 아이템 사용 어빌리티 공용 쿨다운 GameplayEffect(Duration).
 * 지속시간은 고정하지 않고 어빌리티가 SetByCaller(Data.Cooldown)로 주입한다.
 * 부여 태그(쿨다운 식별)도 고정하지 않고, 어빌리티가 아이템별로 DynamicGrantedTags에 주입한다(쿨다운 분리용).
 */
UCLASS()
class PROJECTFT_API UFTGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Cooldown();
};
