#include "FTCraftRecipeEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTCraftRecipeListObject.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTCraftRecipeEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTCraftRecipeListObject* RecipeObject = Cast<UFTCraftRecipeListObject>(ListItemObject);
	if (!RecipeObject)
	{
		return;
	}

	const FTCraftRecipeStruct& Recipe = RecipeObject->GetRecipe();
	FString RequiredItems;

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT(", ");
		}

		RequiredItems += FString::Printf(TEXT("%s x%d"), *Ingredient.ItemID.ToString(), Ingredient.Count);
	}

	RecipeNameText->SetText(FText::FromName(Recipe.RecipeID));
	RequiredItemsText->SetText(FText::FromString(RequiredItems));
	ResultItemText->SetText(FText::FromString(
		FString::Printf(TEXT("%s x%d"), *Recipe.ResultItemID.ToString(), Recipe.ResultCount)));

	const FSlateColor TextColor = RecipeObject->CanCraft()
		? FSlateColor(FLinearColor::White)
		: FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));

	RecipeNameText->SetColorAndOpacity(TextColor);
	RequiredItemsText->SetColorAndOpacity(TextColor);
	ResultItemText->SetColorAndOpacity(TextColor);
}
