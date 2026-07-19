// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FTRaidResultType.generated.h"

UENUM(BlueprintType)
enum class EFTRaidResultType : uint8
{
	Escaped UMETA(DisplayName = "Escaped"),
	Failed UMETA(DisplayName = "Failed")
};
