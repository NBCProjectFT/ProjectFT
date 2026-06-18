// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Struct/FTStatStruct.h"
#include "FTPlayerStatStruct.generated.h"

/**
 * 플레이어 스탯 묶음(데이터). 로직/통지는 UFTPlayerStatComponent가 담당한다.
 * 자원형(체력·스태미나)과 속성형(이동속도·손재주)을 함께 담는다.
 */
USTRUCT(BlueprintType)
struct FFTPlayerStatStruct
{
	GENERATED_BODY()

	// 체력(자원형).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat")
	FFTStatStruct Health;

	// 스태미나(자원형).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat")
	FFTStatStruct Stamina;

	// 이동 속도(cm/s, 속성형). CharacterMovement에 반영해 사용한다(연동은 별도 단계).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat", meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.0f;

	// 손재주(속성형): 진열대 훔치기 등 채널형 작업 속도 배수(1.0 = 기본).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat", meta = (ClampMin = "0.0"))
	float Dexterity = 1.0f;
};
