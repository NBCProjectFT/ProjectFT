#include "FTHubCraftTestWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "FTCraftRecipeListObject.h"
#include "FTStorageItemListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubWorkbench.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTHubCraftTestWidget::InitializeCraftTest(AFTHubWorkbench* InHubWorkbench, UFTInventoryComponent* InPlayerInventory)
{
	HubWorkbench = InHubWorkbench;
	PlayerInventory = InPlayerInventory;
	SelectedRecipe = nullptr;
	RefreshAll();
}

void UFTHubCraftTestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_CraftRecipes)
	{
		LV_CraftRecipes->OnItemClicked().RemoveAll(this);
		LV_CraftRecipes->OnItemClicked().AddUObject(this, &UFTHubCraftTestWidget::HandleRecipeClicked);
	}

	if (BTN_Craft)
	{
		BTN_Craft->OnClicked.RemoveDynamic(this, &UFTHubCraftTestWidget::HandleCraftClicked);
		BTN_Craft->OnClicked.AddDynamic(this, &UFTHubCraftTestWidget::HandleCraftClicked);
		BTN_Craft->SetIsEnabled(false);
	}

	UpdateSelectedRecipeDetails();
	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubCraftTestWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubCraftTestWidget::HandleCloseClicked);
	}

	RefreshAll();
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
		RecipeObject->Initialize(Recipe, HubWorkbench->CanCraftRecipe(Recipe, PlayerInventory));
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

		const int32 PlayerCount = PlayerInventory
			? PlayerInventory->GetItemQuantity(Ingredient.ItemID)
			: 0;
		const int32 StorageCount = HubWorkbench->GetHubStorage()
			? HubWorkbench->GetHubStorage()->GetStorageItemCount(Ingredient.ItemID)
			: 0;
		const int32 TotalCount = PlayerCount + StorageCount;

		RequiredItems += FString::Printf(
			TEXT("%s x%d / 보유 %d"),
			*Ingredient.ItemID.ToString(),
			Ingredient.Count,
			TotalCount);
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

	if (HubWorkbench->TryCraftRecipe(SelectedRecipe->GetRecipe().RecipeID, PlayerInventory))
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
