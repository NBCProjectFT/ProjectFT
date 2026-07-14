#include "FTUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "FTCountdownEscapeWidget.h"
#include "FTEscapedRaidWidget.h"
#include "FTFailWidget.h"
#include "FTInventoryWidget.h"
#include "FTMainMenuWidget.h"
#include "HubUI/FTHubCraftTestWidget.h"
#include "HubUI/FTHubMainWidget.h"
#include "HubUI/FTHubMarketPanelWidget.h"
#include "HubUI/FTHubQuestPanelWidget.h"
#include "HubUI/FTHubShopPanelWidget.h"
#include "HubUI/FTHubStorageWidget.h"
#include "HubUI/FTRaidSelectWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "../ViewModel/FTCraftingViewModel.h"
#include "../ViewModel/FTHUDViewModel.h"
#include "../ViewModel/FTInventoryViewModel.h"
#include "../ViewModel/FTQuestViewModel.h"
#include "../ViewModel/FTSettlementViewModel.h"
#include "../ViewModel/FTHubStorageViewModel.h"
#include "../ViewModel/FTRaidSelectViewModel.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubTerminal.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
}

void UFTUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HUDViewModel = NewObject<UFTHUDViewModel>(this);
	InventoryViewModel = NewObject<UFTInventoryViewModel>(this);
	CraftingViewModel = NewObject<UFTCraftingViewModel>(this);
	QuestViewModel = NewObject<UFTQuestViewModel>(this);
	SettlementViewModel = NewObject<UFTSettlementViewModel>(this);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	UIMessageListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ObjectiveProgressChanged, this, &ThisClass::HandleObjectiveProgressChanged));
	UIMessageListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ObjectiveCompleted, this, &ThisClass::HandleObjectiveCompleted));
}

void UFTUIManagerSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	for (FGameplayMessageListenerHandle& ListenerHandle : UIMessageListenerHandles)
	{
		if (ListenerHandle.IsValid())
		{
			MessageSubsystem.UnregisterListener(ListenerHandle);
		}
	}
	UIMessageListenerHandles.Reset();

	Super::Deinitialize();
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
		TSubclassOf<UFTMainMenuWidget> MainMenuWidgetClass = UFTAssetManager::Get().GetMainMenuWidgetClass();
		if (!MainMenuWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Main menu widget class is not set in UI data."));
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
		TSubclassOf<UFTCountdownEscapeWidget> CountdownEscapeWidgetClass = UFTAssetManager::Get().GetCountdownEscapeWidgetClass();
		if (!CountdownEscapeWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Countdown escape widget class is not set in UI data."));
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

void UFTUIManagerSubsystem::ShowEscapedRaid()
{
	if (EscapedRaidWidget && EscapedRaidWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Escaped raid widget was not created because PlayerController is missing."));
		return;
	}

	if (!EscapedRaidWidget)
	{
		TSubclassOf<UFTEscapedRaidWidget> EscapedRaidWidgetClass = UFTAssetManager::Get().GetEscapedRaidWidgetClass();
		if (!EscapedRaidWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Escaped raid widget class is not set in UI data."));
			return;
		}

		EscapedRaidWidget = CreateWidget<UFTEscapedRaidWidget>(PlayerController, EscapedRaidWidgetClass);
		if (!EscapedRaidWidget)
		{
			return;
		}
	}

	HideCountdownEscape();
	EscapedRaidWidget->AddToViewport(40);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(EscapedRaidWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	UE_LOG(LogFTUI, Log, TEXT("Escaped raid widget shown. Widget=%s Class=%s"),
		*GetNameSafe(EscapedRaidWidget),
		*GetNameSafe(EscapedRaidWidget->GetClass()));
}

void UFTUIManagerSubsystem::HideEscapedRaid()
{
	if (EscapedRaidWidget)
	{
		EscapedRaidWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}

		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
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

	if (!InventoryWidget)
	{
		TSubclassOf<UFTInventoryWidget> InventoryWidgetClass = UFTAssetManager::Get().GetInventoryWidgetClass();
		if (!InventoryWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Inventory widget class is not set in UI data."));
			return;
		}

		InventoryWidget = CreateWidget<UFTInventoryWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			return;
		}
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



void UFTUIManagerSubsystem::ShowCrafting(UFTInventoryComponent* PlayerInventory, UFTInventoryComponent* StorageInventory)
{
	if (!PlayerInventory)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Craft widget was not created because PlayerInventory is missing."));
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

	TSubclassOf<UFTHubCraftTestWidget> CraftWidgetClass = UFTAssetManager::Get().GetHubCraftWidgetClass();

	if (!CraftWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Craft widget class is not set in UI data."));
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

	HubCraftWidget->InitializeCraftTest(PlayerInventory, StorageInventory, CraftingViewModel);
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



void UFTUIManagerSubsystem::ShowStorage(AFTHubStorage* HubStorage, UFTInventoryComponent* PlayerInventory)
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

	TSubclassOf<UFTHubStorageWidget> StorageWidgetClass = UFTAssetManager::Get().GetHubStorageWidgetClass();

	if (!StorageWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Storage widget class is not set in UI data."));
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

void UFTUIManagerSubsystem::ShowRaidSelect(AFTHubRaidEntrance* RaidEntrance, UFTInventoryComponent* PlayerInventory)
{
	if (!RaidEntrance)
	{
		return;
	}

	if (RaidSelectWidget && RaidSelectWidget->IsInViewport())
	{
		HideRaidSelect();
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	TSubclassOf<UFTRaidSelectWidget> WidgetClass = RaidEntrance->GetRaidSelectWidgetClass();
	if (!PlayerController || !WidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Raid select widget was not opened. PlayerController=%s WidgetClass=%s"),
			*GetNameSafe(PlayerController),
			*GetNameSafe(WidgetClass));
		return;
	}

	if (!RaidSelectWidget || !RaidSelectWidget->IsA(WidgetClass))
	{
		RaidSelectWidget = CreateWidget<UFTRaidSelectWidget>(PlayerController, WidgetClass);
	}
	if (!RaidSelectWidget)
	{
		return;
	}

	if (!RaidSelectViewModel)
	{
		RaidSelectViewModel = NewObject<UFTRaidSelectViewModel>(this);
	}
	RaidSelectViewModel->Initialize(RaidEntrance, PlayerInventory);
	RaidSelectWidget->InitializeRaidSelect(RaidSelectViewModel);
	RaidSelectWidget->AddToViewport(20);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(RaidSelectWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void UFTUIManagerSubsystem::HideRaidSelect()
{
	if (RaidSelectWidget)
	{
		RaidSelectWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}

void UFTUIManagerSubsystem::ShowHubMain(
	AFTHubTerminal* HubTerminal,
	AFTHubStorage* HubStorage,
	UFTInventoryComponent* PlayerInventory
)
{
	if (HubMainWidget && HubMainWidget->IsInViewport())
	{
		HideHubMain();
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub main widget was not created because PlayerController is missing."));
		return;
	}

	TSubclassOf<UFTHubMainWidget> HubMainWidgetClass = nullptr;
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		HubMainWidgetClass = UFTAssetManager::GetSubclass(GameData->HubMainWidgetClass);
	}

	if (!HubMainWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub main widget class is not set in game data."));
		return;
	}

	if (!HubMainWidget || !HubMainWidget->IsA(HubMainWidgetClass))
	{
		HubMainWidget = CreateWidget<UFTHubMainWidget>(PlayerController, HubMainWidgetClass);
		if (!HubMainWidget)
		{
			return;
		}
	}

	UFTShopSubsystem* ShopSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTShopSubsystem>()
		: nullptr;
	if (ShopSubsystem)
	{
		ShopSubsystem->ConfigureHubStorage(HubStorage);
	}
	else
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub shop and market panels will be empty because ShopSubsystem is missing."));
	}

	HubMainWidget->InitializeHubMain(HubTerminal, ShopSubsystem, PlayerInventory);

	if (UFTHubQuestPanelWidget* QuestPanelWidget = HubMainWidget->GetQuestPanelWidget())
	{
		UGameInstance* GameInstance = GetGameInstance();
		QuestPanelWidget->InitializeQuestPanel(GameInstance ? GameInstance->GetSubsystem<UFTObjectiveSubsystem>() : nullptr, PlayerInventory);
	}
	else
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub quest panel is missing from HubMainWidget."));
	}

	if (UFTHubMarketPanelWidget* MarketPanelWidget = HubMainWidget->GetMarketPanelWidget())
	{
		MarketPanelWidget->InitializeMarketPanel(ShopSubsystem, PlayerInventory);
	}
	else
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub market panel is missing from HubMainWidget."));
	}

	if (UFTHubShopPanelWidget* ShopPanelWidget = HubMainWidget->GetShopPanelWidget())
	{
		ShopPanelWidget->InitializeShopPanel(ShopSubsystem, PlayerInventory);
	}
	else
	{
		UE_LOG(LogFTUI, Warning, TEXT("Hub shop panel is missing from HubMainWidget."));
	}

	HubMainWidget->AddToViewport(20);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(HubMainWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
	HubMainWidget->SetKeyboardFocus();
}

void UFTUIManagerSubsystem::HideHubMain()
{
	if (HubMainWidget)
	{
		HubMainWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}

		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}

bool UFTUIManagerSubsystem::IsHubMainOpen() const
{
	return HubMainWidget && HubMainWidget->IsInViewport();
}

void UFTUIManagerSubsystem::ShowFailScreen()
{
	if (FailWidget && FailWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Fail widget was not created because PlayerController is missing."));
		return;
	}

	if (!FailWidget)
	{
		TSubclassOf<UFTFailWidget> FailWidgetClass = UFTAssetManager::Get().GetFailWidgetClass();
		if (!FailWidgetClass)
		{
			UE_LOG(LogFTUI, Warning, TEXT("Fail widget class is not set in active game data. Set DA_FTGameData.FailWidgetClass to WBP_FailWidget."));
			return;
		}

		FailWidget = CreateWidget<UFTFailWidget>(PlayerController, FailWidgetClass);
		if (!FailWidget)
		{
			return;
		}
	}

	HideCountdownEscape();
	HideEscapedRaid();
	FailWidget->AddToViewport(40);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(FailWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	UE_LOG(LogFTUI, Log, TEXT("Fail widget shown. Widget=%s Class=%s"),
		*GetNameSafe(FailWidget),
		*GetNameSafe(FailWidget->GetClass()));
}

void UFTUIManagerSubsystem::HideFailScreen()
{
	if (FailWidget)
	{
		FailWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}

		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}

void UFTUIManagerSubsystem::ShowSettlementScreen()
{
}

APlayerController* UFTUIManagerSubsystem::GetPrimaryPlayerController() const
{
	const UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetFirstLocalPlayerController() : nullptr;
}

void UFTUIManagerSubsystem::HandleObjectiveProgressChanged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (!HUDViewModel)
	{
		return;
	}

	const int32 ProgressPercent = FMath::RoundToInt(FMath::Clamp(Payload.Value, 0.0f, 1.0f) * 100.0f);
	HUDViewModel->SetObjectiveText(FText::FromString(FString::Printf(TEXT("Quest %s %d%%"), *Payload.QuestId.ToString(), ProgressPercent)));
}

void UFTUIManagerSubsystem::HandleObjectiveCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (!HUDViewModel)
	{
		return;
	}

	HUDViewModel->SetObjectiveText(FText::FromString(FString::Printf(TEXT("Quest %s Complete"), *Payload.QuestId.ToString())));
}
