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
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "GameFramework/Pawn.h"

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
	if (IsInventoryOpen())
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (AFTPlayerCharacter* PlayerChar = Cast<AFTPlayerCharacter>(PlayerController->GetPawn()))
	{
		PlayerChar->SetInventoryOpen(true, false);
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

		if (AFTPlayerCharacter* PlayerChar = Cast<AFTPlayerCharacter>(PlayerController->GetPawn()))
		{
			PlayerChar->SetInventoryOpen(false, false);
		}
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
