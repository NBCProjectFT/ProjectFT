// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FTGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFT_API AFTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidStart();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidFail();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidEscape();
};
