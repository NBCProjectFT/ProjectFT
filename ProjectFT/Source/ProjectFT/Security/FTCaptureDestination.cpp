// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCaptureDestination.h"

#include "Components/SceneComponent.h"

AFTCaptureDestination::AFTCaptureDestination()
{
	PrimaryActorTick.bCanEverTick = false;

	// 위치만 필요한 마커라 씬 컴포넌트 루트만 둔다(비주얼은 BP에서 메시를 붙여도 됨).
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
