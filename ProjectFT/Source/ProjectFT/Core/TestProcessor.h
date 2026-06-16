// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayMessageProcessor.h"
#include "TestProcessor.generated.h"

struct FTTestStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UTestProcessor : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	
protected:
	virtual void StartListening() override;
	virtual void StopListening() override;
	
public:
	void OnTestMessage(FGameplayTag Channel, const FTTestStruct& Payload);
	
};
