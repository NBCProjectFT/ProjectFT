#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTCraftingWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void RefreshRecipeList();

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void UpdateRequiredMaterials(FName RecipeId);

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void RequestCraft(FName RecipeId);
};
