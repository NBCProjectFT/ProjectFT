#pragma once

#include "CoreMinimal.h"
#include "FTCraftIngredientStruct.generated.h"

USTRUCT(BlueprintType)
struct FTCraftIngredientStruct
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;
};