// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FTCapturable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFTCapturable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTFT_API IFTCapturable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool CanBeCapturedBy(AActor* CaptorActor) = 0;
	virtual void BeginCapture(AActor* CaptorActor) = 0;
	virtual void EndCapture(AActor* CaptorActor) = 0;
};
