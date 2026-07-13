#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Struct/FTSavedInventoryItemStruct.h"
#include "FTSavedInventoryStateStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTSavedInventoryStateStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Save")
	TArray<FFTSavedInventoryItemStruct> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Save")
	TArray<FName> QuickSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Save")
	float MaxWeight = 100.0f;
};
