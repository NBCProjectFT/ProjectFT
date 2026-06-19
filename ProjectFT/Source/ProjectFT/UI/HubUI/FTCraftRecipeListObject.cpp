#include "FTCraftRecipeListObject.h"

void UFTCraftRecipeListObject::Initialize(const FTCraftRecipeStruct& InRecipe, bool bInCanCraft)
{
	Recipe = InRecipe;
	bCanCraft = bInCanCraft;
}

const FTCraftRecipeStruct& UFTCraftRecipeListObject::GetRecipe() const
{
	return Recipe;
}

bool UFTCraftRecipeListObject::CanCraft() const
{
	return bCanCraft;
}
