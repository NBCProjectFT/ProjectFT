#include "FTHubWorkbench.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "FTHubStorage.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/HubUI/FTHubCraftTestWidget.h"
#include "ProjectFT/ViewModel/FTCraftingViewModel.h"

AFTHubWorkbench::AFTHubWorkbench()
	: CraftRecipeDataTable(nullptr)
	, HubStorage(nullptr)
	, HubCraftTestWidget(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	auto AddDefaultRecipe = [this](FName RecipeID, TArray<FTCraftIngredientStruct> RequiredItems, FName ResultItemID)
	{
		FTCraftRecipeStruct Recipe;
		Recipe.RecipeID = RecipeID;
		Recipe.RequiredItems = MoveTemp(RequiredItems);
		Recipe.ResultItemID = ResultItemID;
		Recipe.ResultCount = 1;
		DefaultCraftRecipes.Add(Recipe);
	};

	AddDefaultRecipe(
		TEXT("Make_HardBaguette"),
		{
			{ TEXT("ID_Healing_Baguette"), 1 },
			{ TEXT("ID_Common_HairDryer"), 1 }
		},
		TEXT("ID_Healing_Baguette"));

	AddDefaultRecipe(
		TEXT("Make_DryIce"),
		{
			{ TEXT("ID_Healing_Ice"), 1 },
			{ TEXT("ID_Common_HairDryer"), 1 }
		},
		TEXT("ID_Common_DryIce"));

	AddDefaultRecipe(
		TEXT("Make_FrozenTuna"),
		{
			{ TEXT("ID_Healing_FreshTuna"), 1 },
			{ TEXT("ID_Healing_Ice"), 1 },
			{ TEXT("ID_Healing_Salt"), 1 },
			{ TEXT("ID_Common_DryIce"), 1 }
		},
		TEXT("ID_Healing_FreshTuna"));

	AddDefaultRecipe(
		TEXT("Make_SoapWater"),
		{
			{ TEXT("ID_Healing_Water"), 1 },
			{ TEXT("ID_Common_Soap"), 1 }
		},
		TEXT("ID_Common_SoapWater"));

	AddDefaultRecipe(
		TEXT("Make_BubbleGun"),
		{
			{ TEXT("ID_Common_SoapWater"), 1 },
			{ TEXT("ID_Weapon_Taser"), 1 }
		},
		TEXT("ID_Weapon_ThrowItem"));

	AddDefaultRecipe(
		TEXT("Make_WaterGun"),
		{
			{ TEXT("ID_Weapon_ThrowItem"), 1 },
			{ TEXT("ID_Healing_Water"), 1 }
		},
		TEXT("ID_Weapon_Taser"));

	AddDefaultRecipe(
		TEXT("Make_ColaMentosBomb"),
		{
			{ TEXT("ID_Healing_Cola"), 1 },
			{ TEXT("ID_Healing_Mentos"), 1 }
		},
		TEXT("ID_Weapon_ThrowItem"));
}

void AFTHubWorkbench::BeginPlay()
{
	Super::BeginPlay();
}

bool AFTHubWorkbench::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Hub Workbench Interacted"));

	UFTInventoryComponent* PlayerInventory = FindPlayerInventory(Interactor);
	PrintAllRecipes(PlayerInventory);
	OpenCraftWidget(Interactor);

	return true;
}

FText AFTHubWorkbench::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("작업대 사용"));
}

void AFTHubWorkbench::OpenCraftWidget(AActor* Interactor)
{
	if (!HubCraftTestWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HubCraftTestWidgetClass is not assigned."));
		return;
	}

	APlayerController* PlayerController = nullptr;

	if (APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		PlayerController = Cast<APlayerController>(InteractorPawn->GetController());
	}

	if (!PlayerController)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	if (!PlayerController)
	{
		return;
	}
	if (HubCraftTestWidget && HubCraftTestWidget->IsInViewport())
	{
		CloseCraftWidget();
		return;
	}
	UFTInventoryComponent* PlayerInventory = FindPlayerInventory(Interactor);

	if (!HubCraftTestWidget)
	{
		HubCraftTestWidget = CreateWidget<UFTHubCraftTestWidget>(PlayerController, HubCraftTestWidgetClass);
		if (!HubCraftTestWidget)
		{
			return;
		}
	}

	if (!CraftingViewModel)
	{
		CraftingViewModel = NewObject<UFTCraftingViewModel>(this);
	}

	HubCraftTestWidget->InitializeCraftTest(this, PlayerInventory, CraftingViewModel);

	if (!HubCraftTestWidget->IsInViewport())
	{
		HubCraftTestWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubCraftTestWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

void AFTHubWorkbench::PrintAllRecipes(UFTInventoryComponent* PlayerInventory) const
{
	TArray<FTCraftRecipeStruct> Recipes;
	GetCraftRecipes(Recipes);

	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecipeID: %s"), *Recipe.RecipeID.ToString());

		for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
		{
			const int32 OwnedCount = GetCombinedItemCount(PlayerInventory, Ingredient.ItemID);

			UE_LOG(LogTemp, Warning, TEXT("Required: %s x%d / Owned: %d"),
				*Ingredient.ItemID.ToString(),
				Ingredient.Count,
				OwnedCount);
		}

		UE_LOG(LogTemp, Warning, TEXT("CanCraft: %s"),
			CanCraftRecipe(Recipe, PlayerInventory) ? TEXT("true") : TEXT("false"));
	}
}

bool AFTHubWorkbench::CanCraftRecipe(const FTCraftRecipeStruct& Recipe, UFTInventoryComponent* PlayerInventory) const
{
	if (!PlayerInventory && !HubStorage)
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
	if (bUseDefaultCraftRecipes)
	{
		return DefaultCraftRecipes.FindByPredicate([RecipeID](const FTCraftRecipeStruct& Recipe)
		{
			return Recipe.RecipeID == RecipeID;
		});
	}

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
	if (HubCraftTestWidget && HubCraftTestWidget->IsInViewport())
	{
		HubCraftTestWidget->RemoveFromParent();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void AFTHubWorkbench::GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const
{
	OutRecipes.Reset();

	if (bUseDefaultCraftRecipes)
	{
		OutRecipes = DefaultCraftRecipes;
		return;
	}

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

UFTInventoryComponent* AFTHubWorkbench::FindPlayerInventory(AActor* Interactor) const
{
	if (Interactor)
	{
		if (UFTInventoryComponent* PlayerInventory = Interactor->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	const APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

	if (!PlayerController)
	{
		return nullptr;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (UFTInventoryComponent* PlayerInventory = Pawn->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	return PlayerController->FindComponentByClass<UFTInventoryComponent>();
}

int32 AFTHubWorkbench::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
{
	int32 Count = 0;

	if (PlayerInventory)
	{
		Count += PlayerInventory->GetItemQuantity(ItemID);
	}

	if (HubStorage)
	{
		Count += HubStorage->GetStorageItemCount(ItemID);
	}

	return Count;
}

bool AFTHubWorkbench::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count)
{
	if (ItemID.IsNone() || Count <= 0 || GetCombinedItemCount(PlayerInventory, ItemID) < Count)
	{
		return false;
	}

	int32 RemainingCount = Count;

	if (PlayerInventory)
	{
		const int32 PlayerCount = PlayerInventory->GetItemQuantity(ItemID);
		const int32 RemoveFromPlayer = FMath::Min(PlayerCount, RemainingCount);

		if (RemoveFromPlayer > 0 && PlayerInventory->RemoveItem(ItemID, RemoveFromPlayer))
		{
			RemainingCount -= RemoveFromPlayer;
		}
	}

	if (RemainingCount > 0 && HubStorage)
	{
		const int32 StorageCount = HubStorage->GetStorageItemCount(ItemID);
		const int32 RemoveFromStorage = FMath::Min(StorageCount, RemainingCount);

		if (RemoveFromStorage > 0 && HubStorage->RemoveStorageItem(ItemID, RemoveFromStorage))
		{
			RemainingCount -= RemoveFromStorage;
		}
	}

	return RemainingCount <= 0;
}
