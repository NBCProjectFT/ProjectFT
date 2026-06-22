// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_SpeedBuff.generated.h"

/**
 * 일정 시간 이동속도를 올리는 GameplayEffect(Duration).
 * 같은 근원 재적용 = 갱신(시간 리셋), 다른 근원 = 별도 스택. (Aggregate by Source + Refresh)
 */
UCLASS()
class PROJECTFT_API UFTGE_SpeedBuff : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_SpeedBuff();
};
