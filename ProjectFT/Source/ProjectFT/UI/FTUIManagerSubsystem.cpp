#include "FTUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "FTCountdownEscapeWidget.h"
#include "FTInventoryWidget.h"
#include "FTMainMenuWidget.h"
#include "HubUI/FTHubCraftTestWidget.h"
#include "HubUI/FTHubStorageWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "../ViewModel/FTCraftingViewModel.h"
#include "../ViewModel/FTHUDViewModel.h"
#include "../ViewModel/FTInventoryViewModel.h"
#include "../ViewModel/FTQuestViewModel.h"
#include "../ViewModel/FTSettlementViewModel.h"
#include "../ViewModel/FTHubStorageViewModel.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubWorkbench.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	// Temporary fallback path. This should move to FTUIDataAsset when UI config is separated.
	const TCHAR* CountdownEscapeWidgetFallbackPath = TEXT("/Game/UI/Escaping/WBP_CountDownEscape.WBP_CountDownEscape_C");
}

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

void UFTUIManagerSubsystem::ShowMainMenu()
{
	if (MainMenuWidget && MainMenuWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Main menu widget was not created because PlayerController is missing."));
		return;
	}

	if (!MainMenuWidget)
	{
		TSubclassOf<UFTMainMenuWidget> MainMenuWidgetClass = nullptr;
		if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
		{
			MainMenuWidgetClass = UFTAssetManager::GetSubclass(GameData->MainMenuWidgetClass);
		}

		if (!MainMenuWidgetClass)
		{
			MainMenuWidgetClass = LoadClass<UFTMainMenuWidget>(
				nullptr,
				TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C"));
		}

		if (!MainMenuWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Main menu widget class is not set."));
			return;
		}

		MainMenuWidget = CreateWidget<UFTMainMenuWidget>(PlayerController, MainMenuWidgetClass);
		if (!MainMenuWidget)
		{
			return;
		}
	}

	MainMenuWidget->AddToViewport(10);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	UE_LOG(LogFTUI, Log, TEXT("Main menu shown. UIOnly input applied. PlayerController=%s Widget=%s"),
		*GetNameSafe(PlayerController),
		*GetNameSafe(MainMenuWidget));
}

void UFTUIManagerSubsystem::HideMainMenu(bool bKeepMouseCursor)
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		// 메뉴는 UIOnly 입력을 사용하므로, 레벨 이동 전에 반드시 게임 입력으로 되돌린다.
		// 이 복구가 빠지면 다음 맵에서 Pawn/Controller가 정상이어도 입력이 UI 포커스에 묶인 것처럼 보일 수 있다.
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}

		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = bKeepMouseCursor;

		UE_LOG(LogFTUI, Log, TEXT("Main menu hidden. GameOnly input restored. KeepMouseCursor=%s PlayerController=%s"),
			bKeepMouseCursor ? TEXT("true") : TEXT("false"),
			*GetNameSafe(PlayerController));
	}
	else
	{
		UE_LOG(LogFTUI, Warning, TEXT("Main menu hidden without PlayerController; input mode could not be restored."));
	}
}

void UFTUIManagerSubsystem::ShowCountdownEscape()
{
	if (CountdownEscapeWidget && CountdownEscapeWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Countdown escape widget was not created because PlayerController is missing."));
		return;
	}

	if (!CountdownEscapeWidget)
	{
		TSubclassOf<UFTCountdownEscapeWidget> CountdownEscapeWidgetClass =
			LoadClass<UFTCountdownEscapeWidget>(nullptr, CountdownEscapeWidgetFallbackPath);
		if (!CountdownEscapeWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Countdown escape widget class could not be loaded. Path=%s"),
				CountdownEscapeWidgetFallbackPath);
			return;
		}

		CountdownEscapeWidget = CreateWidget<UFTCountdownEscapeWidget>(PlayerController, CountdownEscapeWidgetClass);
		if (!CountdownEscapeWidget)
		{
			return;
		}
	}

	CountdownEscapeWidget->AddToViewport(30);
}

void UFTUIManagerSubsystem::HideCountdownEscape()
{
	if (CountdownEscapeWidget)
	{
		CountdownEscapeWidget->RemoveFromParent();
		CountdownEscapeWidget->ResetCountdown();
	}
}

void UFTUIManagerSubsystem::SetCountdownEscapeRemainingTime(float RemainingTime)
{
	if (!CountdownEscapeWidget || !CountdownEscapeWidget->IsInViewport())
	{
		ShowCountdownEscape();
	}

	if (CountdownEscapeWidget)
	{
		CountdownEscapeWidget->SetRemainingTime(RemainingTime);
	}
}

void UFTUIManagerSubsystem::ShowInventory()
{
	if (IsInventoryOpen())
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

	// 뷰모델을 현재 플레이어의 인벤토리 컴포넌트에 연결한다. 이 연결이 없으면 위젯은 떠도 아이템이 항상 비어 있다
	// (ViewModel::NotifyChanged가 LinkedInventory 없이 즉시 return하기 때문).
	if (InventoryViewModel)
	{
		UFTInventoryComponent* InventoryComp = nullptr;
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			InventoryComp = Pawn->FindComponentByClass<UFTInventoryComponent>();
		}

		if (InventoryComp)
		{
			if (InventoryViewModel->GetLinkedInventory() != InventoryComp)
			{
				InventoryViewModel->Initialize(InventoryComp); // 델리게이트 바인딩 + 최초 동기화
			}
			else
			{
				InventoryViewModel->NotifyChanged(); // 이미 연결됨 → 최신값으로 갱신 후 UI 브로드캐스트
			}
		}
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

	if (InventoryViewModel)
	{
		InventoryViewModel->ClearSelection();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;

	}
}

void UFTUIManagerSubsystem::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		HideInventory();
	}
	else
	{
		ShowInventory();
	}
}

bool UFTUIManagerSubsystem::IsInventoryOpen() const
{
	return InventoryWidget && InventoryWidget->IsInViewport();
}

void UFTUIManagerSubsystem::ShowCrafting()
{
	UE_LOG(LogFTUI, Warning, TEXT("ShowCrafting called without a workbench context."));
}

void UFTUIManagerSubsystem::ShowCrafting(AFTHubWorkbench* HubWorkbench, UFTInventoryComponent* PlayerInventory, TSubclassOf<UFTHubCraftTestWidget> FallbackWidgetClass)
{
	if (!HubWorkbench)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Craft widget was not created because HubWorkbench is missing."));
		return;
	}

	if (HubCraftWidget && HubCraftWidget->IsInViewport())
	{
		HideCrafting();
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Craft widget was not created because PlayerController is missing."));
		return;
	}

	TSubclassOf<UFTHubCraftTestWidget> CraftWidgetClass = nullptr;
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		CraftWidgetClass = UFTAssetManager::GetSubclass(GameData->HubCraftWidgetClass);
	}

	if (!CraftWidgetClass)
	{
		CraftWidgetClass = FallbackWidgetClass;
	}

	if (!CraftWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Craft widget class is not set in game data or fallback actor."));
		return;
	}

	if (!HubCraftWidget || !HubCraftWidget->IsA(CraftWidgetClass))
	{
		HubCraftWidget = CreateWidget<UFTHubCraftTestWidget>(PlayerController, CraftWidgetClass);
		if (!HubCraftWidget)
		{
			return;
		}
	}

	if (!CraftingViewModel)
	{
		CraftingViewModel = NewObject<UFTCraftingViewModel>(this);
	}

	HubCraftWidget->InitializeCraftTest(HubWorkbench, PlayerInventory, CraftingViewModel);
	HubCraftWidget->AddToViewport(20);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(HubCraftWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void UFTUIManagerSubsystem::HideCrafting()
{
	if (HubCraftWidget)
	{
		HubCraftWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}

void UFTUIManagerSubsystem::ShowStorage()
{
	UE_LOG(LogFTUI, Warning, TEXT("ShowStorage called without a storage context."));
}

void UFTUIManagerSubsystem::ShowStorage(AFTHubStorage* HubStorage, UFTInventoryComponent* PlayerInventory, TSubclassOf<UFTHubStorageWidget> FallbackWidgetClass)
{
	if (!HubStorage)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Storage widget was not created because HubStorage is missing."));
		return;
	}

	if (HubStorageWidget && HubStorageWidget->IsInViewport())
	{
		HideStorage();
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Storage widget was not created because PlayerController is missing."));
		return;
	}

	TSubclassOf<UFTHubStorageWidget> StorageWidgetClass = nullptr;
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		StorageWidgetClass = UFTAssetManager::GetSubclass(GameData->HubStorageWidgetClass);
	}

	if (!StorageWidgetClass)
	{
		StorageWidgetClass = FallbackWidgetClass;
	}

	if (!StorageWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Storage widget class is not set in game data or fallback actor."));
		return;
	}

	if (!HubStorageWidget || !HubStorageWidget->IsA(StorageWidgetClass))
	{
		HubStorageWidget = CreateWidget<UFTHubStorageWidget>(PlayerController, StorageWidgetClass);
		if (!HubStorageWidget)
		{
			return;
		}
	}

	if (!HubStorageViewModel)
	{
		HubStorageViewModel = NewObject<UFTHubStorageViewModel>(this);
	}

	HubStorageWidget->InitializeStorageWidget(HubStorage, PlayerInventory, HubStorageViewModel);
	HubStorageWidget->AddToViewport(20);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(HubStorageWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void UFTUIManagerSubsystem::HideStorage()
{
	if (HubStorageWidget)
	{
		HubStorageWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
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
