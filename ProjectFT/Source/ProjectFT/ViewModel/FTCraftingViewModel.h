#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTCraftingViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTCraftingViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	TArray<FName> RecipeList;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	FName SelectedRecipe = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	bool bCanCraft = false;

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void NotifyChanged();
};
