// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Haste.generated.h"

/**
 * 헤이스트(Haste) 버프 GameplayEffect(Duration). 지속 동안 이동속도를 올리고 대상에게 State.Buff.Haste 태그를 부여한다.
 * 버프/디버프는 종류당 대상에 하나만 유지(AggregateByTarget+limit1) — 재적용 시 지속시간만 갱신된다.
 */
UCLASS()
class PROJECTFT_API UFTGE_Haste : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Haste();
};
