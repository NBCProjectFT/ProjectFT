#pragma once

#include "CoreMinimal.h"
#include "FTMessagePayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTMessagePayloadStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Message")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Message")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Message")
	float Value = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Message")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Message")
	FName QuestId = NAME_None;
};
