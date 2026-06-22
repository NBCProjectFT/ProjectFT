// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Poison.generated.h"

/**
 * 일정 시간 동안 주기적으로 체력을 깎는 디버프 GameplayEffect(Duration + Period).
 * 매 주기 Health에 음수 모디파이어를 '실행'하므로 PostGameplayEffectExecute에서 사망 판정이 걸린다.
 */
UCLASS()
class PROJECTFT_API UFTGE_Poison : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Poison();
};
