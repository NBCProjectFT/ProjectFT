// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTCaptureDestination.generated.h"

/**
 * 경비가 붙잡은 플레이어를 이송하는 목적지 마커(데드 바이 데이라이트의 갈고리 역할). 레벨에 1개 이상 배치한다.
 * UFTGA_Grab이 잡는 순간 가장 가까운 이 액터를 골라 MoveTo하고, 도달하면 탈출 실패로 판정한다.
 */
UCLASS()
class PROJECTFT_API AFTCaptureDestination : public AActor
{
	GENERATED_BODY()

public:
	AFTCaptureDestination();

	// 이 지점에 "도달"로 인정할 반경(cm). 경비 MoveTo의 AcceptanceRadius로 쓰인다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 150.0f;
};
