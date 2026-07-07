#include "FTHubWorkbench.h"

#include "Engine/DataTable.h"
#include "FTHubActorUtils.h"
#include "FTHubStorage.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubWorkbench::AFTHubWorkbench()
	: CraftRecipeDataTable(nullptr)
	, HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTHubWorkbench::BeginPlay()
{
	Super::BeginPlay();
}

bool AFTHubWorkbench::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Hub Workbench Interacted"));

	OpenCraftWidget(Interactor);

	return true;
}

FText AFTHubWorkbench::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("작업대 사용"));
}

void AFTHubWorkbench::OpenCraftWidget(AActor* Interactor)
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowCrafting(this, FTHubActorUtils::FindPlayerInventory(this, Interactor));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Craft widget was not opened because UIManager is missing."));
}

bool AFTHubWorkbench::CanCraftRecipe(const FTCraftRecipeStruct& Recipe, UFTInventoryComponent* PlayerInventory) const
{
	if (!PlayerInventory && !GetStorageInventory())
	{
		return false;
	}

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (GetCombinedItemCount(PlayerInventory, Ingredient.ItemID) < Ingredient.Count)
		{
			return false;
		}
	}

	return true;
}

const FTCraftRecipeStruct* AFTHubWorkbench::FindRecipeByID(FName RecipeID) const
{
	if (!CraftRecipeDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("CraftRecipeDataTable is not assigned."));
		return nullptr;
	}

	return CraftRecipeDataTable->FindRow<FTCraftRecipeStruct>(RecipeID, TEXT("FindRecipeByID"));
}

bool AFTHubWorkbench::TryCraftRecipe(FName RecipeID, UFTInventoryComponent* PlayerInventory)
{
	const FTCraftRecipeStruct* Recipe = FindRecipeByID(RecipeID);

	if (!Recipe || !PlayerInventory || !CanCraftRecipe(*Recipe, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft Failed: %s"), *RecipeID.ToString());
		return false;
	}

	for (const FTCraftIngredientStruct& Ingredient : Recipe->RequiredItems)
	{
		if (!ConsumeCombinedItem(PlayerInventory, Ingredient.ItemID, Ingredient.Count))
		{
			UE_LOG(LogTemp, Warning, TEXT("Craft Failed: %s"), *RecipeID.ToString());
			return false;
		}
	}

	if (!PlayerInventory->AddItem(Recipe->ResultItemID, Recipe->ResultCount))
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft Reward Failed: %s"), *RecipeID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Craft Success: %s"), *RecipeID.ToString());
	return true;
}

void AFTHubWorkbench::CloseCraftWidget()
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->HideCrafting();
	}
}

void AFTHubWorkbench::GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const
{
	OutRecipes.Reset();

	if (!CraftRecipeDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("CraftRecipeDataTable is not assigned."));
		return;
	}

	TArray<FTCraftRecipeStruct*> RecipeRows;
	CraftRecipeDataTable->GetAllRows<FTCraftRecipeStruct>(TEXT("GetCraftRecipes"), RecipeRows);

	for (const FTCraftRecipeStruct* Recipe : RecipeRows)
	{
		if (Recipe)
		{
			OutRecipes.Add(*Recipe);
		}
	}
}

AFTHubStorage* AFTHubWorkbench::GetHubStorage() const
{
	return HubStorage;
}

UFTStorageSubsystem* AFTHubWorkbench::GetStorageSubsystem() const
{
	return GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;
}

UFTInventoryComponent* AFTHubWorkbench::GetStorageInventory() const
{
	return HubStorage ? HubStorage->GetStorageInventory() : nullptr;
}

int32 AFTHubWorkbench::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
{
	const UFTStorageSubsystem* StorageSubsystem = GetStorageSubsystem();
	return StorageSubsystem
		? StorageSubsystem->GetCombinedItemCount(PlayerInventory, GetStorageInventory(), ItemID)
		: (PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0);
}

bool AFTHubWorkbench::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count)
{
	UFTStorageSubsystem* StorageSubsystem = GetStorageSubsystem();
	return StorageSubsystem && StorageSubsystem->ConsumeCombinedItem(PlayerInventory, GetStorageInventory(), ItemID, Count);
}
