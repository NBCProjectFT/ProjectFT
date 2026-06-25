// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Slow.generated.h"

/**
 * 슬로우 디버프 GameplayEffect(Duration). 지속 동안 이동속도를 낮추고 대상에게 State.Debuff.Slow 태그를 부여한다.
 * 지속시간은 아이템 데이터가 SetByCaller(Data.Duration)로 주입한다.
 * 버프/디버프는 종류당 대상에 하나만 유지(AggregateByTarget+limit1) — 재적용 시 지속시간만 갱신된다. (UFTGE_Haste의 거울)
 */
UCLASS()
class PROJECTFT_API UFTGE_Slow : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Slow();
};
