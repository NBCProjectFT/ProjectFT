#include "FTHubWorkbench.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "FTHubStorage.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/HubUI/FTHubCraftTestWidget.h"

AFTHubWorkbench::AFTHubWorkbench()
	: CraftRecipeDataTable(nullptr)
	, HubStorage(nullptr)
	, HubCraftTestWidget(nullptr)
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

	PrintAllRecipes();
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
	if (!HubCraftTestWidget)
	{
		HubCraftTestWidget = CreateWidget<UFTHubCraftTestWidget>(PlayerController, HubCraftTestWidgetClass);
		if (!HubCraftTestWidget)
		{
			return;
		}

		HubCraftTestWidget->InitializeCraftTest(this);
	}

	if (!HubCraftTestWidget->IsInViewport())
	{
		HubCraftTestWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubCraftTestWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

void AFTHubWorkbench::PrintAllRecipes() const
{
	TArray<FTCraftRecipeStruct> Recipes;
	GetCraftRecipes(Recipes);

	for (const FTCraftRecipeStruct& Recipe : Recipes)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecipeID: %s"), *Recipe.RecipeID.ToString());

		for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
		{
			const int32 OwnedCount = HubStorage
				? HubStorage->GetStorageItemCount(Ingredient.ItemID)
				: 0;

			UE_LOG(LogTemp, Warning, TEXT("Required: %s x%d / Owned: %d"),
				*Ingredient.ItemID.ToString(),
				Ingredient.Count,
				OwnedCount);
		}

		UE_LOG(LogTemp, Warning, TEXT("CanCraft: %s"),
			CanCraftRecipe(Recipe) ? TEXT("true") : TEXT("false"));
	}
}

bool AFTHubWorkbench::CanCraftRecipe(const FTCraftRecipeStruct& Recipe) const
{
	if (!HubStorage)
	{
		return false;
	}

	for (const FTCraftIngredientStruct& Ingredient : Recipe.RequiredItems)
	{
		if (HubStorage->GetStorageItemCount(Ingredient.ItemID) < Ingredient.Count)
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

bool AFTHubWorkbench::TryCraftRecipe(FName RecipeID)
{
	const FTCraftRecipeStruct* Recipe = FindRecipeByID(RecipeID);

	if (!Recipe || !HubStorage || !CanCraftRecipe(*Recipe))
	{
		UE_LOG(LogTemp, Warning, TEXT("Craft Failed: %s"), *RecipeID.ToString());
		return false;
	}

	for (const FTCraftIngredientStruct& Ingredient : Recipe->RequiredItems)
	{
		HubStorage->RemoveStorageItem(Ingredient.ItemID, Ingredient.Count);
	}

	HubStorage->AddStorageItem(Recipe->ResultItemID, Recipe->ResultCount);

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