#include "FTCraftRecipeEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FTCraftRecipeListObject.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

namespace
{
	FText ResolveItemName(const UObject* WorldContextObject, const FName ItemID)
	{
		const UFTItemDataAsset* ItemDataAsset = UFTItemFunctionLibrary::FindItemData(WorldContextObject, ItemID);
		return ItemDataAsset && !ItemDataAsset->ItemData.ItemName.IsEmpty()
			? ItemDataAsset->ItemData.ItemName
			: FText::FromName(ItemID);
	}
}

void UFTCraftRecipeEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTCraftRecipeListObject* RecipeObject = Cast<UFTCraftRecipeListObject>(ListItemObject);
	if (!RecipeObject)
	{
		return;
	}

	const FTCraftRecipeStruct& Recipe = RecipeObject->GetRecipe();
	FText RecipeTitle = FText::FromName(Recipe.ResultItemID);
	const UFTItemDataAsset* ItemDataAsset = UFTItemFunctionLibrary::FindItemData(this, Recipe.ResultItemID);
	if (ItemDataAsset)
	{
		RecipeTitle = ItemDataAsset->ItemData.ItemName.IsEmpty()
			? FText::FromName(Recipe.ResultItemID)
			: ItemDataAsset->ItemData.ItemName;
	}

	FString RequiredItems;

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT(", ");
		}

		RequiredItems += FString::Printf(TEXT("%s x%d"), *ResolveItemName(this, Ingredient.ItemID).ToString(), Ingredient.Count);
	}

	RecipeNameText->SetText(RecipeTitle);
	if (IMG_ResultItemIcon && ItemDataAsset)
	{
		if (UTexture2D* IconTexture = ItemDataAsset->ItemData.ItemIcon.LoadSynchronous())
		{
			IMG_ResultItemIcon->SetBrushFromTexture(IconTexture);
		}
	}
	if (RequiredItemsText)
	{
		RequiredItemsText->SetText(FText::FromString(RequiredItems));
	}
	if (ResultItemText)
	{
		ResultItemText->SetText(Recipe.ResultCount > 1
			? FText::FromString(FString::Printf(TEXT("x%d"), Recipe.ResultCount))
			: FText::GetEmpty());
	}
	if (TXT_ResultCount)
	{
		TXT_ResultCount->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Recipe.ResultCount)));
	}
	if (TXT_CraftableState)
	{
		TXT_CraftableState->SetText(RecipeObject->CanCraft()
			? FText::FromString(TEXT("제작가능"))
			: FText::FromString(TEXT("제작불가")));
	}

	const FSlateColor TextColor = RecipeObject->CanCraft()
		? FSlateColor(FLinearColor::White)
		: FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));

	RecipeNameText->SetColorAndOpacity(TextColor);
	if (RequiredItemsText)
	{
		RequiredItemsText->SetColorAndOpacity(TextColor);
	}
	if (ResultItemText)
	{
		ResultItemText->SetColorAndOpacity(TextColor);
	}
	if (TXT_CraftableState)
	{
		TXT_CraftableState->SetColorAndOpacity(RecipeObject->CanCraft()
			? FSlateColor(FLinearColor(0.0f, 0.7f, 0.2f, 1.0f))
			: FSlateColor(FLinearColor(1.0f, 0.4f, 0.1f, 1.0f)));
	}
}
