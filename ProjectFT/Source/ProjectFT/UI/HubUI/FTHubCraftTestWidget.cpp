#include "FTHubCraftTestWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "FTCraftRecipeListObject.h"
#include "FTItemTileListObject.h"
#include "FTStorageItemListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
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

	if (CHK_CraftableOnly)
	{
		CHK_CraftableOnly->OnCheckStateChanged.RemoveDynamic(this, &UFTHubCraftTestWidget::HandleCraftableOnlyChanged);
		CHK_CraftableOnly->OnCheckStateChanged.AddDynamic(this, &UFTHubCraftTestWidget::HandleCraftableOnlyChanged);
	}

	if (EDT_SearchRecipe)
	{
		EDT_SearchRecipe->OnTextChanged.RemoveDynamic(this, &UFTHubCraftTestWidget::HandleSearchRecipeTextChanged);
		EDT_SearchRecipe->OnTextChanged.AddDynamic(this, &UFTHubCraftTestWidget::HandleSearchRecipeTextChanged);
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
	if (!LV_CraftRecipes)
	{
		return;
	}

	RefreshStorageItems();
	RefreshCraftRecipes();
}

void UFTHubCraftTestWidget::RefreshStorageItems()
{
	UListView* StorageItemsView = GetStorageItemsView();
	if (!StorageItemsView)
	{
		return;
	}

	StorageItemsView->ClearListItems();

	if (!HubWorkbench || !HubWorkbench->GetHubStorage())
	{
		return;
	}

	for (const FTStorageItemStruct& StorageItem : HubWorkbench->GetHubStorage()->GetStorageItems())
	{
		UFTStorageItemListObject* ItemObject = NewObject<UFTStorageItemListObject>(this);
		ItemObject->Initialize(StorageItem);
		StorageItemsView->AddItem(ItemObject);
	}
}

void UFTHubCraftTestWidget::RefreshCraftRecipes()
{
	if (!LV_CraftRecipes)
	{
		return;
	}

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

	int32 VisibleRecipeCount = 0;
	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		const bool bCanCraft = HubWorkbench->CanCraftRecipe(Recipe, PlayerInventory);
		if (!ShouldShowRecipe(Recipe, bCanCraft))
		{
			continue;
		}

		UFTCraftRecipeListObject* RecipeObject = NewObject<UFTCraftRecipeListObject>(this);
		RecipeObject->Initialize(Recipe, bCanCraft);
		LV_CraftRecipes->AddItem(RecipeObject);
		++VisibleRecipeCount;

		if (Recipe.RecipeID == SelectedRecipeID)
		{
			SelectedRecipe = RecipeObject;
			LV_CraftRecipes->SetItemSelection(RecipeObject, true);
		}
	}

	if (TXT_RecipeCount)
	{
		TXT_RecipeCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), VisibleRecipeCount, Recipes.Num())));
	}

	UpdateSelectedRecipeDetails();
}

UListView* UFTHubCraftTestWidget::GetStorageItemsView() const
{
	return TV_StorageItems ? Cast<UListView>(TV_StorageItems) : LV_StorageItems;
}

bool UFTHubCraftTestWidget::ShouldShowRecipe(const FTCraftRecipeStruct& Recipe, const bool bCanCraft) const
{
	if (CHK_CraftableOnly && CHK_CraftableOnly->IsChecked() && !bCanCraft)
	{
		return false;
	}

	if (!EDT_SearchRecipe)
	{
		return true;
	}

	const FString SearchText = EDT_SearchRecipe->GetText().ToString().TrimStartAndEnd();
	if (SearchText.IsEmpty())
	{
		return true;
	}

	return Recipe.RecipeID.ToString().Contains(SearchText, ESearchCase::IgnoreCase)
		|| Recipe.ResultItemID.ToString().Contains(SearchText, ESearchCase::IgnoreCase);
}

int32 UFTHubCraftTestWidget::GetOwnedIngredientCount(const FName ItemID) const
{
	const int32 PlayerCount = PlayerInventory
		? PlayerInventory->GetItemQuantity(ItemID)
		: 0;
	const int32 StorageCount = HubWorkbench && HubWorkbench->GetHubStorage()
		? HubWorkbench->GetHubStorage()->GetStorageItemCount(ItemID)
		: 0;

	return PlayerCount + StorageCount;
}

const UFTItemDataAsset* UFTHubCraftTestWidget::FindItemData(const FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	const FPrimaryAssetId AssetID(FName("FTItemItem"), ItemID);

	UObject* AssetObject = AssetManager.GetPrimaryAssetObject(AssetID);
	if (!AssetObject)
	{
		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetID);
		if (AssetPath.IsValid())
		{
			AssetObject = AssetPath.TryLoad();
		}
	}

	return Cast<UFTItemDataAsset>(AssetObject);
}

void UFTHubCraftTestWidget::RefreshRequiredItemTiles()
{
	if (!TV_RequiredItems)
	{
		return;
	}

	TV_RequiredItems->ClearListItems();

	if (!SelectedRecipe)
	{
		return;
	}

	for (const FTCraftIngredientStruct& Ingredient : SelectedRecipe->GetRecipe().RequiredItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(Ingredient, GetOwnedIngredientCount(Ingredient.ItemID));
		TV_RequiredItems->AddItem(ItemObject);
	}
}

void UFTHubCraftTestWidget::UpdateSelectedRecipeDetails()
{
	if (!TXT_SelectedRecipeName || !BTN_Craft)
	{
		return;
	}

	if (!SelectedRecipe)
	{
		TXT_SelectedRecipeName->SetText(FText::FromString(TEXT("Select Recipe")));
		if (TXT_RequiredItems)
		{
			TXT_RequiredItems->SetText(FText::GetEmpty());
		}
		if (TXT_ResultItem)
		{
			TXT_ResultItem->SetText(FText::GetEmpty());
		}
		if (TXT_SelectedRecipeTier)
		{
			TXT_SelectedRecipeTier->SetText(FText::GetEmpty());
		}
		if (TXT_SelectedRecipeDescription)
		{
			TXT_SelectedRecipeDescription->SetText(FText::GetEmpty());
		}
		if (TXT_CraftTime)
		{
			TXT_CraftTime->SetText(FText::GetEmpty());
		}
		if (TXT_CraftAmount)
		{
			TXT_CraftAmount->SetText(FText::GetEmpty());
		}
		if (TV_RequiredItems)
		{
			TV_RequiredItems->ClearListItems();
		}
		BTN_Craft->SetIsEnabled(false);
		return;
	}

	const FTCraftRecipeStruct& Recipe = SelectedRecipe->GetRecipe();
	const UFTItemDataAsset* ResultItemData = FindItemData(Recipe.ResultItemID);
	FString RequiredItems;

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT("\n");
		}

		const int32 TotalCount = GetOwnedIngredientCount(Ingredient.ItemID);

		RequiredItems += FString::Printf(
			TEXT("%s %d / %d"),
			*Ingredient.ItemID.ToString(),
			TotalCount,
			Ingredient.Count);
	}

	TXT_SelectedRecipeName->SetText(ResultItemData && !ResultItemData->ItemData.ItemName.IsEmpty()
		? ResultItemData->ItemData.ItemName
		: FText::FromName(Recipe.RecipeID));
	if (TXT_RequiredItems)
	{
		TXT_RequiredItems->SetText(FText::FromString(RequiredItems));
	}
	if (TXT_ResultItem)
	{
		TXT_ResultItem->SetText(FText::FromString(
			FString::Printf(TEXT("Result: %s x%d"), *Recipe.ResultItemID.ToString(), Recipe.ResultCount)));
	}
	if (TXT_SelectedRecipeTier)
	{
		TXT_SelectedRecipeTier->SetText(FText::FromString(TEXT("Tier 1")));
	}
	if (TXT_SelectedRecipeDescription)
	{
		TXT_SelectedRecipeDescription->SetText(ResultItemData
			? ResultItemData->ItemData.ItemDescription
			: FText::GetEmpty());
	}
	if (TXT_CraftTime)
	{
		TXT_CraftTime->SetText(FText::FromString(TEXT("1 sec")));
	}
	if (TXT_CraftAmount)
	{
		TXT_CraftAmount->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Recipe.ResultCount)));
	}
	if (IMG_ResultItemIcon && ResultItemData)
	{
		if (UTexture2D* IconTexture = ResultItemData->ItemData.ItemIcon.LoadSynchronous())
		{
			IMG_ResultItemIcon->SetBrushFromTexture(IconTexture);
		}
	}
	RefreshRequiredItemTiles();
	BTN_Craft->SetIsEnabled(SelectedRecipe->CanCraft());
}

void UFTHubCraftTestWidget::HandleRecipeClicked(UObject* Item)
{
	SelectedRecipe = Cast<UFTCraftRecipeListObject>(Item);
	UpdateSelectedRecipeDetails();
}

void UFTHubCraftTestWidget::HandleCraftableOnlyChanged(const bool bIsChecked)
{
	(void)bIsChecked;
	RefreshCraftRecipes();
}

void UFTHubCraftTestWidget::HandleSearchRecipeTextChanged(const FText& Text)
{
	(void)Text;
	RefreshCraftRecipes();
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
		UpdateSelectedRecipeDetails();
	}
}

void UFTHubCraftTestWidget::HandleCloseClicked()
{
	if (HubWorkbench)
	{
		HubWorkbench->CloseCraftWidget();
	}
}
