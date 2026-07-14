#pragma once

#include "CoreMinimal.h"
#include "FTSavedInventoryItemStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTSavedInventoryItemStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Save")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Save")
	int32 Quantity = 0;
};
