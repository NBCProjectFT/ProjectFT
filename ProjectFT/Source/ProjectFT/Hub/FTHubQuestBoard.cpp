#include "FTHubQuestBoard.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubShop.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/UI/HubUI/FTHubQuestTestWidget.h"

AFTHubQuestBoard::AFTHubQuestBoard()
	: QuestDataTable(nullptr)
	, HubStorage(nullptr)
	, HubShop(nullptr)
	, HubQuestTestWidget(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTHubQuestBoard::BeginPlay()
{
	Super::BeginPlay();
	
	for (const FName& QuestID : InitialQuestIDs)
	{
		UnlockQuest(QuestID);
	}
}

bool AFTHubQuestBoard::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Hub Quest Board Interacted"));

	OpenQuestWidget(Interactor);
	return true;
}

FText AFTHubQuestBoard::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("퀘스트 게시판 보기"));
}

void AFTHubQuestBoard::OpenQuestWidget(AActor* Interactor)
{
	if (HubQuestTestWidget && HubQuestTestWidget->IsInViewport())
	{
		CloseQuestWidget();
		return;
	}

	if (!HubQuestTestWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HubQuestTestWidgetClass is not assigned."));
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

	UFTInventoryComponent* PlayerInventory = FindPlayerInventory(Interactor);

	if (!HubQuestTestWidget)
	{
		HubQuestTestWidget = CreateWidget<UFTHubQuestTestWidget>(
			PlayerController,
			HubQuestTestWidgetClass
		);

		if (!HubQuestTestWidget)
		{
			return;
		}
	}

	HubQuestTestWidget->InitializeQuestTest(this, PlayerInventory);

	if (!HubQuestTestWidget->IsInViewport())
	{
		HubQuestTestWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubQuestTestWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

void AFTHubQuestBoard::CloseQuestWidget()
{
	if (HubQuestTestWidget && HubQuestTestWidget->IsInViewport())
	{
		HubQuestTestWidget->RemoveFromParent();
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

bool AFTHubQuestBoard::CanCompleteQuest(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const
{
	if (!PlayerInventory && !HubStorage)
	{
		return false;
	}

	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (GetCombinedItemCount(PlayerInventory, RequiredItem.ItemID) < RequiredItem.Count)
		{
			return false;
		}
	}

	return true;
}

bool AFTHubQuestBoard::TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory)
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);
	
	if (CompletedQuestIDs.Contains(QuestID))
	{
		return false;
	}
	if (!Quest || !PlayerInventory || !CanCompleteQuest(*Quest, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest Complete Failed: %s"), *QuestID.ToString());
		return false;
	}

	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		if (!ConsumeCombinedItem(PlayerInventory, RequiredItem.ItemID, RequiredItem.Count))
		{
			UE_LOG(LogTemp, Warning, TEXT("Quest Complete Failed: %s"), *QuestID.ToString());
			return false;
		}
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest->RewardItems)
	{
		if (!PlayerInventory->AddItem(RewardItem.ItemID, RewardItem.Count))
		{
			UE_LOG(LogTemp, Warning, TEXT("Quest Reward Failed: %s"), *QuestID.ToString());
			return false;
		}
	}

	if (HubShop)
	{
		for (const FName& ShopItemID : Quest->UnlockedShopItemIDs)
		{
			HubShop->UnlockShopItem(ShopItemID);
		}
	}

	CompletedQuestIDs.Add(QuestID);
	AvailableQuestIDs.Remove(QuestID);

	for (const FName& NextQuestID : Quest->NextQuestIDs)
	{
		UnlockQuest(NextQuestID);
	}

	UE_LOG(LogTemp, Warning, TEXT("Quest Complete Success: %s"), *QuestID.ToString());
	return true;

}

void AFTHubQuestBoard::GetQuestList(TArray<FTQuestStruct>& OutQuests) const
{
	OutQuests.Reset();

	if (!QuestDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestDataTable is not assigned."));
		return;
	}

	for (const FName& QuestID : AvailableQuestIDs)
	{
		const FTQuestStruct* Quest = QuestDataTable->FindRow<FTQuestStruct>(
			QuestID,
			TEXT("GetQuestList")
		);

		if (Quest)
		{
			OutQuests.Add(*Quest);
		}
	}
}

AFTHubStorage* AFTHubQuestBoard::GetHubStorage() const
{
	return HubStorage;
}

const FTQuestStruct* AFTHubQuestBoard::FindQuestByID(FName QuestID) const
{
	if (!QuestDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestDataTable is not assigned."));
		return nullptr;
	}

	return QuestDataTable->FindRow<FTQuestStruct>(QuestID, TEXT("FindQuestByID"));
}

bool AFTHubQuestBoard::IsQuestAvailable(FName QuestID) const
{
	return AvailableQuestIDs.Contains(QuestID);
}

bool AFTHubQuestBoard::IsQuestCompleted(FName QuestID) const
{
	return CompletedQuestIDs.Contains(QuestID);
}

EFTQuestStateType AFTHubQuestBoard::GetQuestState(FName QuestID) const
{
	if (CompletedQuestIDs.Contains(QuestID))
	{
		return EFTQuestStateType::Completed;
	}

	if (AvailableQuestIDs.Contains(QuestID))
	{
		return EFTQuestStateType::Available;
	}

	return EFTQuestStateType::Locked;
}

void AFTHubQuestBoard::UnlockQuest(FName QuestID)
{
	if (QuestID.IsNone())
	{
		return;
	}

	if (CompletedQuestIDs.Contains(QuestID))
	{
		return;
	}

	AvailableQuestIDs.Add(QuestID);
}

UFTInventoryComponent* AFTHubQuestBoard::FindPlayerInventory(AActor* Interactor) const
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

int32 AFTHubQuestBoard::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
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

bool AFTHubQuestBoard::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count)
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
