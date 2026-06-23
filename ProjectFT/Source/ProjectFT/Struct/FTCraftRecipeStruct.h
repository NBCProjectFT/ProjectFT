#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FTCraftIngredientStruct.h"
#include "FTCraftRecipeStruct.generated.h"

USTRUCT(BlueprintType)
struct FTCraftRecipeStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTCraftIngredientStruct> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ResultItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ResultCount = 1;
};