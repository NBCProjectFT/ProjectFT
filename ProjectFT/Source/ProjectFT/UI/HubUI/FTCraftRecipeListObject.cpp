#include "FTCraftRecipeListObject.h"

#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"

namespace
{
	FText ResolveCraftItemName(const UObject* WorldContextObject, const FName ItemID)
	{
		const UFTItemDataAsset* ItemData = UFTItemFunctionLibrary::FindItemData(WorldContextObject, ItemID);
		return ItemData && !ItemData->ItemData.ItemName.IsEmpty()
			? ItemData->ItemData.ItemName
			: FText::FromName(ItemID);
	}
}

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

FText UFTCraftRecipeListObject::GetRecipeNameText() const
{
	return ResolveCraftItemName(this, Recipe.ResultItemID);
}

FText UFTCraftRecipeListObject::GetResultCountText() const
{
	return FText::FromString(FString::Printf(TEXT("x%d"), Recipe.ResultCount));
}

TSoftObjectPtr<UTexture2D> UFTCraftRecipeListObject::GetResultItemIcon() const
{
	const UFTItemDataAsset* ItemData = UFTItemFunctionLibrary::FindItemData(this, Recipe.ResultItemID);
	return ItemData ? ItemData->ItemData.ItemIcon : nullptr;
}
