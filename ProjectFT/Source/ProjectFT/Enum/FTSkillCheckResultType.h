// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FTSkillCheckResultType.generated.h"

/** 스킬체크 판정 결과. */
UENUM(BlueprintType)
enum class EFTSkillCheckResultType : uint8
{
	// 판정 없음(스킬체크가 중단/취소됨).
	None UMETA(DisplayName = "None"),
	// 실패(성공존 밖에서 입력했거나 시간 초과).
	Miss UMETA(DisplayName = "Miss"),
	// 성공(성공존 안).
	Good UMETA(DisplayName = "Good"),
	// 대성공(더 좁은 great존 안 — 보너스 진행).
	Great UMETA(DisplayName = "Great")
};
