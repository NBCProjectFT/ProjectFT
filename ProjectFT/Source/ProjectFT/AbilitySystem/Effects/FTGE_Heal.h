// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Heal.generated.h"

/**
 * 즉시 체력을 회복하는 GameplayEffect(Instant). 향후 효과는 BP GE 에셋으로 데이터 저작 가능.
 */
UCLASS()
class PROJECTFT_API UFTGE_Heal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Heal();
};
