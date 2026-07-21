#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTCraftRecipeListObject.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class PROJECTFT_API UFTCraftRecipeListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTCraftRecipeStruct& InRecipe, bool bInCanCraft);
	const FTCraftRecipeStruct& GetRecipe() const;

	UFUNCTION(BlueprintPure, Category = "FT|Craft Recipe")
	bool CanCraft() const;

	UFUNCTION(BlueprintPure, Category = "FT|Craft Recipe|Presentation")
	FText GetRecipeNameText() const;

	UFUNCTION(BlueprintPure, Category = "FT|Craft Recipe|Data")
	int32 GetResultCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Craft Recipe|Presentation")
	TSoftObjectPtr<UTexture2D> GetResultItemIcon() const;

private:
	FTCraftRecipeStruct Recipe;
	bool bCanCraft = false;
};
