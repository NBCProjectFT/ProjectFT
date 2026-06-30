#include "FTUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "FTInventoryWidget.h"
#include "../ViewModel/FTCraftingViewModel.h"
#include "../ViewModel/FTHUDViewModel.h"
#include "../ViewModel/FTInventoryViewModel.h"
#include "../ViewModel/FTQuestViewModel.h"
#include "../ViewModel/FTSettlementViewModel.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"

void UFTUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HUDViewModel = NewObject<UFTHUDViewModel>(this);
	InventoryViewModel = NewObject<UFTInventoryViewModel>(this);
	CraftingViewModel = NewObject<UFTCraftingViewModel>(this);
	QuestViewModel = NewObject<UFTQuestViewModel>(this);
	SettlementViewModel = NewObject<UFTSettlementViewModel>(this);
}

void UFTUIManagerSubsystem::ShowHUD()
{
}

void UFTUIManagerSubsystem::ShowInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		return;
	}

	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Inventory widget was not created because game data is missing."));
		return;
	}

	if (!InventoryWidget)
	{
		TSubclassOf<UFTInventoryWidget> InventoryWidgetClass = UFTAssetManager::GetSubclass(GameData->InventoryWidgetClass);
		if (!InventoryWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Inventory widget class is not set in game data."));
			return;
		}

		InventoryWidget = CreateWidget<UFTInventoryWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			return;
		}

		// InventoryWidget->SetPaperMaterial(GameData->PaperFlutterMaterial.LoadSynchronous());
	}

	InventoryWidget->AddToViewport(20);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void UFTUIManagerSubsystem::HideInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}

void UFTUIManagerSubsystem::ToggleInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		HideInventory();
		return;
	}

	ShowInventory();
}

void UFTUIManagerSubsystem::ShowCrafting()
{
}

void UFTUIManagerSubsystem::ShowStorage()
{
}

void UFTUIManagerSubsystem::ShowQuestBoard()
{
}

void UFTUIManagerSubsystem::ShowFailScreen()
{
}

void UFTUIManagerSubsystem::ShowSettlementScreen()
{
}

APlayerController* UFTUIManagerSubsystem::GetPrimaryPlayerController() const
{
	const UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetFirstLocalPlayerController() : nullptr;
}
