// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FTStatStruct.generated.h"

/**
 * 자원형 스탯(현재값/최대값). 체력·스태미나처럼 소모되고 회복되는 값에 사용한다.
 * 값만 담는 데이터 구조체이며, 변경/통지 로직은 소유 컴포넌트가 담당한다.
 */
USTRUCT(BlueprintType)
struct FFTStatStruct
{
	GENERATED_BODY()

	// 현재값.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat", meta = (ClampMin = "0.0"))
	float Current = 100.0f;

	// 최대값.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat", meta = (ClampMin = "0.0"))
	float Max = 100.0f;

	// 0~1 비율(Max가 0이면 0).
	float GetFraction() const { return Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f; }

	// 현재값을 [0, Max] 범위로 보정한다.
	void ClampToMax() { Current = FMath::Clamp(Current, 0.0f, Max); }

	bool IsDepleted() const { return Current <= 0.0f; }
	bool IsFull() const { return Current >= Max; }
};
