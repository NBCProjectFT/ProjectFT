// Fill out your copyright notice in the Description page of Project Settings.


#include "FTPlayerController.h"

void AFTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}
