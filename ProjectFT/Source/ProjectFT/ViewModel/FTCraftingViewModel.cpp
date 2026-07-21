#include "FTCraftingViewModel.h"

#include "Engine/Texture2D.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTCraftingSubsystem.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/HubUI/FTCraftRecipeListObject.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"

void UFTCraftingViewModel::Initialize(
	UFTCraftingSubsystem* InCraftingSubsystem,
	UFTInventoryComponent* InPlayerInventory,
	UFTInventoryComponent* InStorageInventory)
{
	UnbindInventoryDelegates();

	CraftingSubsystem = InCraftingSubsystem;
	PlayerInventory = InPlayerInventory;
	StorageInventory = InStorageInventory;
	SelectedRecipeObject = nullptr;
	ClearSelectedRecipeDetails();

	BindInventoryDelegates();
	RefreshAll();
}

TArray<UObject*> UFTCraftingViewModel::GetRecipeObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(RecipeObjects.Num());
	for (UObject* Item : RecipeObjects)
	{
		Result.Add(Item);
	}
	return Result;
}

TArray<UObject*> UFTCraftingViewModel::GetRequiredItemObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(RequiredItemObjects.Num());
	for (UObject* Item : RequiredItemObjects)
	{
		Result.Add(Item);
	}
	return Result;
}

UFTCraftRecipeListObject* UFTCraftingViewModel::GetSelectedRecipeObject() const
{
	return SelectedRecipeObject;
}

FText UFTCraftingViewModel::GetSelectedRecipeNameText() const
{
	return SelectedRecipeNameText;
}

FText UFTCraftingViewModel::GetSelectedRecipeDescriptionText() const
{
	return SelectedRecipeDescriptionText;
}

TSoftObjectPtr<UTexture2D> UFTCraftingViewModel::GetResultItemIcon() const
{
	return ResultItemIcon;
}

bool UFTCraftingViewModel::CanCraftSelectedRecipe() const
{
	return SelectedRecipeObject && SelectedRecipeObject->CanCraft();
}

void UFTCraftingViewModel::RefreshAll()
{
	RefreshRecipes();
	NotifyChanged();
}

void UFTCraftingViewModel::SetCraftableOnly(const bool bInCraftableOnly)
{
	bCraftableOnly = bInCraftableOnly;
	RefreshRecipes();
	NotifyChanged();
}

void UFTCraftingViewModel::SelectRecipeObject(UObject* RecipeObject)
{
	SelectedRecipeObject = Cast<UFTCraftRecipeListObject>(RecipeObject);
	RefreshSelectedRecipeDetails();
	NotifyChanged();
}

bool UFTCraftingViewModel::CraftSelectedRecipe()
{
	if (!CraftingSubsystem || !PlayerInventory || !SelectedRecipeObject)
	{
		return false;
	}

	const FName RecipeID = SelectedRecipeObject->GetRecipe().RecipeID;
	bTransactionInProgress = true;
	if (!CraftingSubsystem->TryCraftRecipe(RecipeID, PlayerInventory, StorageInventory))
	{
		bTransactionInProgress = false;
		return false;
	}
	bTransactionInProgress = false;

	RefreshAll();
	return true;
}

void UFTCraftingViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}

void UFTCraftingViewModel::HandleInventoryChanged()
{
	if (!bTransactionInProgress)
	{
		RefreshAll();
	}
}

void UFTCraftingViewModel::RefreshRecipes()
{
	const FName SelectedRecipeID = SelectedRecipeObject
		? SelectedRecipeObject->GetRecipe().RecipeID
		: NAME_None;

	SelectedRecipeObject = nullptr;
	RecipeObjects.Reset();

	if (!CraftingSubsystem)
	{
		ClearSelectedRecipeDetails();
		return;
	}

	TArray<FTCraftRecipeStruct> Recipes;
	CraftingSubsystem->GetCraftRecipes(Recipes);

	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		const bool bRecipeCanCraft = CraftingSubsystem->CanCraftRecipe(Recipe, PlayerInventory, StorageInventory);
		if (!ShouldShowRecipe(bRecipeCanCraft))
		{
			continue;
		}

		UFTCraftRecipeListObject* RecipeObject = NewObject<UFTCraftRecipeListObject>(this);
		RecipeObject->Initialize(Recipe, bRecipeCanCraft);
		RecipeObjects.Add(RecipeObject);

		if (Recipe.RecipeID == SelectedRecipeID)
		{
			SelectedRecipeObject = RecipeObject;
		}
	}

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
	const UFTItemDataAsset* ResultItemData = UFTItemFunctionLibrary::FindItemData(this, Recipe.ResultItemID);
	SelectedRecipeNameText = ResultItemData && !ResultItemData->ItemData.ItemName.IsEmpty()
		? ResultItemData->ItemData.ItemName
		: FText::FromName(Recipe.ResultItemID);
	SelectedRecipeDescriptionText = ResultItemData ? ResultItemData->ItemData.ItemDescription : FText::GetEmpty();
	ResultItemIcon = ResultItemData ? ResultItemData->ItemData.ItemIcon : nullptr;

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
		ItemObject->SetShowSelectionCheckBox(false);
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

	if (StorageInventory && StorageInventory != PlayerInventory)
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
		StorageInventory->OnInventoryChanged.AddDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}
}

void UFTCraftingViewModel::UnbindInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}

	if (StorageInventory && StorageInventory != PlayerInventory)
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTCraftingViewModel::HandleInventoryChanged);
	}
}

bool UFTCraftingViewModel::ShouldShowRecipe(const bool bRecipeCanCraft) const
{
	return !bCraftableOnly || bRecipeCanCraft;
}

int32 UFTCraftingViewModel::GetOwnedIngredientCount(const FName ItemID) const
{
	return CraftingSubsystem
		? CraftingSubsystem->GetCombinedItemCount(PlayerInventory, StorageInventory, ItemID)
		: (PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0);
}

void UFTCraftingViewModel::ClearSelectedRecipeDetails()
{
	RequiredItemObjects.Reset();
	ResultItemIcon.Reset();
	SelectedRecipeNameText = FText::GetEmpty();
	SelectedRecipeDescriptionText = FText::GetEmpty();
}
