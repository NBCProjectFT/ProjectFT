#include "FTCraftingSubsystem.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Core/FTGameInstance.h"
#include "ProjectFT/Core/FTSaveGame.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

namespace
{
	struct FFTCraftConsumption
	{
		TObjectPtr<UFTInventoryComponent> Inventory = nullptr;
		FName ItemID = NAME_None;
		int32 Count = 0;
	};

	bool RollbackConsumptions(const TArray<FFTCraftConsumption>& AppliedConsumptions)
	{
		bool bRollbackSucceeded = true;
		for (int32 Index = AppliedConsumptions.Num() - 1; Index >= 0; --Index)
		{
			const FFTCraftConsumption& Consumption = AppliedConsumptions[Index];
			if (!Consumption.Inventory || !Consumption.Inventory->AddItem(Consumption.ItemID, Consumption.Count))
			{
				bRollbackSucceeded = false;
				UE_LOG(LogTemp, Error, TEXT("Craft rollback failed: %s x%d"),
					*Consumption.ItemID.ToString(), Consumption.Count);
			}
		}
		return bRollbackSucceeded;
	}
}

void UFTCraftingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureRecipeDataLoaded();
}

bool UFTCraftingSubsystem::FindRecipe(const FName RecipeID, FTCraftRecipeStruct& OutRecipe) const
{
	EnsureRecipeDataLoaded();
	if (!CraftRecipeDataTable || RecipeID.IsNone())
	{
		return false;
	}

	const FTCraftRecipeStruct* Recipe = CraftRecipeDataTable->FindRow<FTCraftRecipeStruct>(RecipeID, TEXT("FindRecipe"));
	if (!Recipe)
	{
		return false;
	}
	if (!IsRecipeUnlocked(RecipeID))
	{
		return false;
	}

	OutRecipe = *Recipe;
	return true;
}

void UFTCraftingSubsystem::GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const
{
	OutRecipes.Reset();
	EnsureRecipeDataLoaded();

	if (!CraftRecipeDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft recipe data table is not configured."));
		return;
	}

	TArray<FTCraftRecipeStruct*> RecipeRows;
	CraftRecipeDataTable->GetAllRows<FTCraftRecipeStruct>(TEXT("GetCraftRecipes"), RecipeRows);
	OutRecipes.Reserve(RecipeRows.Num());

	for (const FTCraftRecipeStruct* Recipe : RecipeRows)
	{
		if (Recipe && IsRecipeUnlocked(Recipe->RecipeID))
		{
			OutRecipes.Add(*Recipe);
		}
	}
}

bool UFTCraftingSubsystem::IsRecipeUnlocked(const FName RecipeID) const
{
	if (RecipeID.IsNone())
	{
		return false;
	}

	const UFTGameInstance* FTGameInstance = Cast<UFTGameInstance>(GetGameInstance());
	const UFTSaveGame* SaveGame = FTGameInstance ? FTGameInstance->CurrentSaveData : nullptr;
	if (SaveGame && !SaveGame->UnlockedRecipes.IsEmpty())
	{
		return SaveGame->UnlockedRecipes.Contains(RecipeID);
	}

	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	return GameData && GameData->bUnlockAllRecipesWhenSaveListEmpty;
}

int32 UFTCraftingSubsystem::GetCombinedItemCount(
	const UFTInventoryComponent* PlayerInventory,
	const UFTInventoryComponent* StorageInventory,
	const FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	const int32 PlayerCount = PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0;
	const int32 StorageCount = StorageInventory && StorageInventory != PlayerInventory
		? StorageInventory->GetItemQuantity(ItemID)
		: 0;
	return PlayerCount + StorageCount;
}

bool UFTCraftingSubsystem::CanCraftRecipe(
	const FTCraftRecipeStruct& Recipe,
	const UFTInventoryComponent* PlayerInventory,
	const UFTInventoryComponent* StorageInventory) const
{
	if (!IsRecipeUnlocked(Recipe.RecipeID) || (!PlayerInventory && !StorageInventory))
	{
		return false;
	}

	TMap<FName, int32> RequiredCounts;
	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (Ingredient.ItemID.IsNone() || Ingredient.Count <= 0)
		{
			return false;
		}

		RequiredCounts.FindOrAdd(Ingredient.ItemID) += Ingredient.Count;
	}

	for (const TPair<FName, int32>& RequiredItem : RequiredCounts)
	{
		if (GetCombinedItemCount(PlayerInventory, StorageInventory, RequiredItem.Key) < RequiredItem.Value)
		{
			return false;
		}
	}

	return true;
}

void UFTCraftingSubsystem::EnsureRecipeDataLoaded() const
{
	if (CraftRecipeDataTable)
	{
		return;
	}

	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft recipe data was not loaded because GameData is missing."));
		return;
	}

	CraftRecipeDataTable = UFTAssetManager::GetAsset(GameData->CraftRecipeDataTable);
	if (!CraftRecipeDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("CraftRecipeDataTable is not assigned in GameData."));
	}
}

bool UFTCraftingSubsystem::TryCraftRecipe(
	const FName RecipeID,
	UFTInventoryComponent* PlayerInventory,
	UFTInventoryComponent* StorageInventory)
{
	FTCraftRecipeStruct Recipe;
	if (!PlayerInventory || !FindRecipe(RecipeID, Recipe)
		|| Recipe.ResultItemID.IsNone() || Recipe.ResultCount <= 0
		|| !CanCraftRecipe(Recipe, PlayerInventory, StorageInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft failed validation: %s"), *RecipeID.ToString());
		return false;
	}

	TMap<FName, int32> RequiredCounts;
	TArray<FName> RequiredItemOrder;
	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (!RequiredCounts.Contains(Ingredient.ItemID))
		{
			RequiredItemOrder.Add(Ingredient.ItemID);
		}
		RequiredCounts.FindOrAdd(Ingredient.ItemID) += Ingredient.Count;
	}

	TArray<FFTCraftConsumption> ConsumptionPlan;
	for (const FName ItemID : RequiredItemOrder)
	{
		int32 RemainingCount = RequiredCounts.FindChecked(ItemID);
		const int32 RemoveFromPlayer = FMath::Min(PlayerInventory->GetItemQuantity(ItemID), RemainingCount);
		if (RemoveFromPlayer > 0)
		{
			ConsumptionPlan.Add({ PlayerInventory, ItemID, RemoveFromPlayer });
			RemainingCount -= RemoveFromPlayer;
		}

		if (RemainingCount > 0 && StorageInventory && StorageInventory != PlayerInventory)
		{
			ConsumptionPlan.Add({ StorageInventory, ItemID, RemainingCount });
		}
	}

	TArray<FFTCraftConsumption> AppliedConsumptions;
	AppliedConsumptions.Reserve(ConsumptionPlan.Num());
	for (const FFTCraftConsumption& Consumption : ConsumptionPlan)
	{
		if (!Consumption.Inventory->RemoveItem(Consumption.ItemID, Consumption.Count))
		{
			RollbackConsumptions(AppliedConsumptions);
			UE_LOG(LogTemp, Warning, TEXT("Craft material consumption failed: %s"), *RecipeID.ToString());
			return false;
		}
		AppliedConsumptions.Add(Consumption);
	}

	if (!PlayerInventory->AddItem(Recipe.ResultItemID, Recipe.ResultCount))
	{
		RollbackConsumptions(AppliedConsumptions);
		UE_LOG(LogTemp, Warning, TEXT("Craft reward failed: %s"), *RecipeID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Craft success: %s"), *RecipeID.ToString());
	return true;
}
