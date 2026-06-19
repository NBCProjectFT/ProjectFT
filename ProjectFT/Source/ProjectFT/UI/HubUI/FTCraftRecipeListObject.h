#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTCraftRecipeListObject.generated.h"

UCLASS()
class PROJECTFT_API UFTCraftRecipeListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTCraftRecipeStruct& InRecipe, bool bInCanCraft);
	const FTCraftRecipeStruct& GetRecipe() const;
	bool CanCraft() const;

private:
	FTCraftRecipeStruct Recipe;
	bool bCanCraft = false;
};
