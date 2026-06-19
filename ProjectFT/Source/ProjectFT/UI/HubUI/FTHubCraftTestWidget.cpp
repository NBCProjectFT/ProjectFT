#include "FTHubCraftTestWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "FTCraftRecipeListObject.h"
#include "FTStorageItemListObject.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubWorkbench.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTHubCraftTestWidget::InitializeCraftTest(AFTHubWorkbench* InHubWorkbench)
{
	HubWorkbench = InHubWorkbench;
	SelectedRecipe = nullptr;
	RefreshAll();
}

void UFTHubCraftTestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LV_CraftRecipes->OnItemClicked().AddUObject(this, &UFTHubCraftTestWidget::HandleRecipeClicked);
	BTN_Craft->OnClicked.AddDynamic(this, &UFTHubCraftTestWidget::HandleCraftClicked);
	BTN_Craft->SetIsEnabled(false);
	UpdateSelectedRecipeDetails();
	if (BTN_Close)
	{
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubCraftTestWidget::HandleCloseClicked);
	}
}

void UFTHubCraftTestWidget::RefreshAll()
{
	if (!LV_StorageItems || !LV_CraftRecipes)
	{
		return;
	}

	RefreshStorageItems();
	RefreshCraftRecipes();
}

void UFTHubCraftTestWidget::RefreshStorageItems()
{
	LV_StorageItems->ClearListItems();

	if (!HubWorkbench || !HubWorkbench->GetHubStorage())
	{
		return;
	}

	for (const FTStorageItemStruct& StorageItem : HubWorkbench->GetHubStorage()->GetStorageItems())
	{
		UFTStorageItemListObject* ItemObject = NewObject<UFTStorageItemListObject>(this);
		ItemObject->Initialize(StorageItem);
		LV_StorageItems->AddItem(ItemObject);
	}
}

void UFTHubCraftTestWidget::RefreshCraftRecipes()
{
	const FName SelectedRecipeID = SelectedRecipe
		? SelectedRecipe->GetRecipe().RecipeID
		: NAME_None;

	SelectedRecipe = nullptr;
	LV_CraftRecipes->ClearListItems();

	if (!HubWorkbench)
	{
		UpdateSelectedRecipeDetails();
		return;
	}

	TArray<FTCraftRecipeStruct> Recipes;
	HubWorkbench->GetCraftRecipes(Recipes);

	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		UFTCraftRecipeListObject* RecipeObject = NewObject<UFTCraftRecipeListObject>(this);
		RecipeObject->Initialize(Recipe, HubWorkbench->CanCraftRecipe(Recipe));
		LV_CraftRecipes->AddItem(RecipeObject);

		if (Recipe.RecipeID == SelectedRecipeID)
		{
			SelectedRecipe = RecipeObject;
			LV_CraftRecipes->SetItemSelection(RecipeObject, true);
		}
	}

	UpdateSelectedRecipeDetails();
}

void UFTHubCraftTestWidget::UpdateSelectedRecipeDetails()
{
	if (!TXT_SelectedRecipeName || !TXT_RequiredItems || !TXT_ResultItem || !BTN_Craft)
	{
		return;
	}

	if (!SelectedRecipe)
	{
		TXT_SelectedRecipeName->SetText(FText::FromString(TEXT("Select Recipe")));
		TXT_RequiredItems->SetText(FText::GetEmpty());
		TXT_ResultItem->SetText(FText::GetEmpty());
		BTN_Craft->SetIsEnabled(false);
		return;
	}

	const FTCraftRecipeStruct& Recipe = SelectedRecipe->GetRecipe();
	FString RequiredItems;

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT("\n");
		}

		RequiredItems += FString::Printf(TEXT("%s x%d"), *Ingredient.ItemID.ToString(), Ingredient.Count);
	}

	TXT_SelectedRecipeName->SetText(FText::FromName(Recipe.RecipeID));
	TXT_RequiredItems->SetText(FText::FromString(RequiredItems));
	TXT_ResultItem->SetText(FText::FromString(
		FString::Printf(TEXT("Result: %s x%d"), *Recipe.ResultItemID.ToString(), Recipe.ResultCount)));
	BTN_Craft->SetIsEnabled(SelectedRecipe->CanCraft());
}

void UFTHubCraftTestWidget::HandleRecipeClicked(UObject* Item)
{
	SelectedRecipe = Cast<UFTCraftRecipeListObject>(Item);
	UpdateSelectedRecipeDetails();
}

void UFTHubCraftTestWidget::HandleCraftClicked()
{
	if (!HubWorkbench || !SelectedRecipe)
	{
		return;
	}

	if (HubWorkbench->TryCraftRecipe(SelectedRecipe->GetRecipe().RecipeID))
	{
		RefreshAll();
	}
}

void UFTHubCraftTestWidget::HandleCloseClicked()
{
	if (HubWorkbench)
	{
		HubWorkbench->CloseCraftWidget();
	}
}
