#include "FTCraftingViewModel.h"

#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubWorkbench.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/HubUI/FTCraftRecipeListObject.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"

void UFTCraftingViewModel::Initialize(AFTHubWorkbench* InHubWorkbench, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegates();

	HubWorkbench = InHubWorkbench;
	PlayerInventory = InPlayerInventory;
	SelectedRecipeObject = nullptr;
	SelectedRecipe = NAME_None;
	bCanCraft = false;
	ClearSelectedRecipeDetails();

	BindInventoryDelegates();
	RefreshAll();
}

const TArray<TObjectPtr<UObject>>& UFTCraftingViewModel::GetStorageItemObjects() const
{
	return StorageItemObjects;
}

const TArray<TObjectPtr<UObject>>& UFTCraftingViewModel::GetRecipeObjects() const
{
	return RecipeObjects;
}

const TArray<TObjectPtr<UObject>>& UFTCraftingViewModel::GetRequiredItemObjects() const
{
	return RequiredItemObjects;
}

FText UFTCraftingViewModel::GetRecipeCountText() const
{
	return RecipeCountText;
}

FText UFTCraftingViewModel::GetSelectedRecipeNameText() const
{
	return SelectedRecipeNameText;
}

FText UFTCraftingViewModel::GetSelectedRecipeTierText() const
{
	return SelectedRecipeTierText;
}

FText UFTCraftingViewModel::GetSelectedRecipeDescriptionText() const
{
	return SelectedRecipeDescriptionText;
}

FText UFTCraftingViewModel::GetCraftTimeText() const
{
	return CraftTimeText;
}

FText UFTCraftingViewModel::GetCraftAmountText() const
{
	return CraftAmountText;
}

FText UFTCraftingViewModel::GetRequiredItemsText() const
{
	return RequiredItemsText;
}

FText UFTCraftingViewModel::GetResultItemText() const
{
	return ResultItemText;
}

UTexture2D* UFTCraftingViewModel::GetResultItemIcon() const
{
	return ResultItemIcon;
}

bool UFTCraftingViewModel::CanCraftSelectedRecipe() const
{
	return SelectedRecipeObject && SelectedRecipeObject->CanCraft();
}

void UFTCraftingViewModel::RefreshAll()
{
	RefreshStorageItems();
	RefreshRecipes();
	NotifyChanged();
}

void UFTCraftingViewModel::SetCraftableOnly(const bool bInCraftableOnly)
{
	bCraftableOnly = bInCraftableOnly;
	RefreshRecipes();
	NotifyChanged();
}

void UFTCraftingViewModel::SetSearchText(const FText& InSearchText)
{
	SearchText = InSearchText;
	RefreshRecipes();
	NotifyChanged();
}

void UFTCraftingViewModel::SelectRecipeObject(UObject* RecipeObject)
{
	SelectedRecipeObject = Cast<UFTCraftRecipeListObject>(RecipeObject);
	SelectedRecipe = SelectedRecipeObject ? SelectedRecipeObject->GetRecipe().RecipeID : NAME_None;
	RefreshSelectedRecipeDetails();
	NotifyChanged();
}

bool UFTCraftingViewModel::CraftSelectedRecipe()
{
	if (!HubWorkbench || !PlayerInventory || !SelectedRecipeObject)
	{
		return false;
	}

	const FName RecipeID = SelectedRecipeObject->GetRecipe().RecipeID;
	if (!HubWorkbench->TryCraftRecipe(RecipeID, PlayerInventory))
	{
		return false;
	}

	RefreshAll();
	return true;
}

void UFTCraftingViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}

void UFTCraftingViewModel::HandleInventoryChanged()
{
	RefreshAll();
}

void UFTCraftingViewModel::RefreshStorageItems()
{
	StorageItemObjects.Reset();

	AFTHubStorage* HubStorage = HubWorkbench ? HubWorkbench->GetHubStorage() : nullptr;
	const UFTStorageSubsystem* StorageSubsystem = HubStorage && HubStorage->GetGameInstance()
		? HubStorage->GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	if (!HubStorage || !StorageSubsystem)
	{
		return;
	}

	TArray<FTStorageItemStruct> StorageItems;
	StorageSubsystem->GetStorageItems(HubStorage->GetStorageInventory(), StorageItems);
	for (const FTStorageItemStruct& StorageItem : StorageItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeItem(StorageItem.ItemID, StorageItem.Count);
		StorageItemObjects.Add(ItemObject);
	}
}

void UFTCraftingViewModel::RefreshRecipes()
{
	const FName SelectedRecipeID = SelectedRecipeObject
		? SelectedRecipeObject->GetRecipe().RecipeID
		: NAME_None;

	SelectedRecipeObject = nullptr;
	RecipeObjects.Reset();
	RecipeList.Reset();

	if (!HubWorkbench)
	{
		ClearSelectedRecipeDetails();
		return;
	}

	TArray<FTCraftRecipeStruct> Recipes;
	HubWorkbench->GetCraftRecipes(Recipes);

	int32 VisibleRecipeCount = 0;
	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		const bool bRecipeCanCraft = HubWorkbench->CanCraftRecipe(Recipe, PlayerInventory);
		if (!ShouldShowRecipe(Recipe, bRecipeCanCraft))
		{
			continue;
		}

		UFTCraftRecipeListObject* RecipeObject = NewObject<UFTCraftRecipeListObject>(this);
		RecipeObject->Initialize(Recipe, bRecipeCanCraft);
		RecipeObjects.Add(RecipeObject);
		RecipeList.Add(Recipe.RecipeID);
		++VisibleRecipeCount;

		if (Recipe.RecipeID == SelectedRecipeID)
		{
			SelectedRecipeObject = RecipeObject;
		}
	}

	RecipeCountText = FText::FromString(FString::Printf(TEXT("%d / %d"), VisibleRecipeCount, Recipes.Num()));
	RefreshSelectedRecipeDetails();
}

void UFTCraftingViewModel::RefreshSelectedRecipeDetails()
{
	if (!SelectedRecipeObject)
	{
		ClearSelectedRecipeDetails();
		return;
	}

	const FTCraftRecipeStruct& Recipe = SelectedRecipeObject->GetRecipe();
	const UFTItemDataAsset* ResultItemData = FindItemData(Recipe.ResultItemID);
	FString RequiredItems;

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT("\n");
		}

		RequiredItems += FString::Printf(
			TEXT("%s %d / %d"),
			*Ingredient.ItemID.ToString(),
			GetOwnedIngredientCount(Ingredient.ItemID),
			Ingredient.Count);
	}

	SelectedRecipe = Recipe.RecipeID;
	bCanCraft = SelectedRecipeObject->CanCraft();
	SelectedRecipeNameText = ResultItemData && !ResultItemData->ItemData.ItemName.IsEmpty()
		? ResultItemData->ItemData.ItemName
		: FText::FromName(Recipe.RecipeID);
	RequiredItemsText = FText::FromString(RequiredItems);
	ResultItemText = FText::FromString(FString::Printf(TEXT("Result: %s x%d"), *Recipe.ResultItemID.ToString(), Recipe.ResultCount));
	SelectedRecipeTierText = FText::FromString(TEXT("Tier 1"));
	SelectedRecipeDescriptionText = ResultItemData ? ResultItemData->ItemData.ItemDescription : FText::GetEmpty();
	CraftTimeText = FText::FromString(TEXT("1 sec"));
	CraftAmountText = FText::FromString(FString::Printf(TEXT("x%d"), Recipe.ResultCount));
	ResultItemIcon = ResultItemData ? ResultItemData->ItemData.ItemIcon.LoadSynchronous() : nullptr;

	RefreshRequiredItemObjects();
}

void UFTCraftingViewModel::RefreshRequiredItemObjects()
{
	RequiredItemObjects.Reset();

	if (!SelectedRecipeObject)
	{
		return;
	}

	for (const FTCraftIngredientStruct& Ingredient : SelectedRecipeObject->GetRecipe().RequiredItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(Ingredient, GetOwnedIngredientCount(Ingredient.ItemID));
		RequiredItemObjects.Add(ItemObject);
	}
}

void UFTCraftingViewModel::BindInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}

	if (HubWorkbench && HubWorkbench->GetHubStorage() && HubWorkbench->GetHubStorage()->GetStorageInventory())
	{
		HubWorkbench->GetHubStorage()->GetStorageInventory()->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
		HubWorkbench->GetHubStorage()->GetStorageInventory()->OnInventoryChanged.AddDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}
}

void UFTCraftingViewModel::UnbindInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}

	if (HubWorkbench && HubWorkbench->GetHubStorage() && HubWorkbench->GetHubStorage()->GetStorageInventory())
	{
		HubWorkbench->GetHubStorage()->GetStorageInventory()->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}
}

bool UFTCraftingViewModel::ShouldShowRecipe(const FTCraftRecipeStruct& Recipe, const bool bRecipeCanCraft) const
{
	if (bCraftableOnly && !bRecipeCanCraft)
	{
		return false;
	}

	const FString SearchString = SearchText.ToString().TrimStartAndEnd();
	if (SearchString.IsEmpty())
	{
		return true;
	}

	return Recipe.RecipeID.ToString().Contains(SearchString, ESearchCase::IgnoreCase)
		|| Recipe.ResultItemID.ToString().Contains(SearchString, ESearchCase::IgnoreCase);
}

int32 UFTCraftingViewModel::GetOwnedIngredientCount(const FName ItemID) const
{
	AFTHubStorage* HubStorage = HubWorkbench ? HubWorkbench->GetHubStorage() : nullptr;
	const UFTStorageSubsystem* StorageSubsystem = HubStorage && HubStorage->GetGameInstance()
		? HubStorage->GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem
		? StorageSubsystem->GetCombinedItemCount(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID)
		: (PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0);
}

const UFTItemDataAsset* UFTCraftingViewModel::FindItemData(const FName ItemID) const
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

void UFTCraftingViewModel::ClearSelectedRecipeDetails()
{
	RequiredItemObjects.Reset();
	ResultItemIcon = nullptr;
	SelectedRecipe = NAME_None;
	bCanCraft = false;
	SelectedRecipeNameText = FText::FromString(TEXT("Select Recipe"));
	SelectedRecipeTierText = FText::GetEmpty();
	SelectedRecipeDescriptionText = FText::GetEmpty();
	CraftTimeText = FText::GetEmpty();
	CraftAmountText = FText::GetEmpty();
	RequiredItemsText = FText::GetEmpty();
	ResultItemText = FText::GetEmpty();
}
