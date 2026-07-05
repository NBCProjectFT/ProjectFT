#include "FTObjectiveSubsystem.h"

#include "../Message/FTGameplayTags.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

bool UFTObjectiveSubsystem::IsObjectiveCompleted() const
{
	for (const FName& RequiredItem : RequiredItems)
	{
		if (!PickedUpRequiredItems.Contains(RequiredItem))
		{
			return false;
		}
	}

	return true;
}

void UFTObjectiveSubsystem::NotifyItemPickedUp(FName ItemId)
{
	if (RequiredItems.Contains(ItemId))
	{
		PickedUpRequiredItems.Add(ItemId);
	}

	if (IsObjectiveCompleted())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		FFTMessagePayloadStruct Payload;
		Payload.ItemId = ItemId;
		Payload.QuestId = CurrentQuestId;
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ObjectiveCompleted, Payload);
	}
}

void UFTObjectiveSubsystem::NotifyEscapeReached()
{
}

void UFTObjectiveSubsystem::ConfigureHubQuests(
	UDataTable* InQuestDataTable,
	AFTHubStorage* InHubStorage,
	const TArray<FName>& InInitialQuestIDs
)
{
	QuestDataTable = InQuestDataTable;
	HubStorage = InHubStorage;

	for (const FName& QuestID : InInitialQuestIDs)
	{
		UnlockQuest(QuestID);
	}
}

bool UFTObjectiveSubsystem::CanCompleteQuest(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const
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

bool UFTObjectiveSubsystem::TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory)
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);

	if (CompletedQuestIDs.Contains(QuestID))
	{
		return false;
	}

	if (!Quest || !PlayerInventory || (!ActiveQuestIDs.Contains(QuestID) && !AvailableQuestIDs.Contains(QuestID)) || !CanCompleteQuest(*Quest, PlayerInventory))
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

	if (UFTShopSubsystem* ShopSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFTShopSubsystem>() : nullptr)
	{
		for (const FName& ShopItemID : Quest->UnlockedShopItemIDs)
		{
			ShopSubsystem->UnlockShopItem(ShopItemID);
		}
	}

	CompletedQuestIDs.Add(QuestID);
	AvailableQuestIDs.Remove(QuestID);
	ActiveQuestIDs.Remove(QuestID);

	for (const FName& NextQuestID : Quest->NextQuestIDs)
	{
		UnlockQuest(NextQuestID);
	}

	UE_LOG(LogTemp, Warning, TEXT("Quest Complete Success: %s"), *QuestID.ToString());
	return true;
}

void UFTObjectiveSubsystem::GetQuestList(TArray<FTQuestStruct>& OutQuests) const
{
	GetQuestListByState(EFTQuestStateType::Available, OutQuests);
}

void UFTObjectiveSubsystem::GetQuestListByState(EFTQuestStateType QuestState, TArray<FTQuestStruct>& OutQuests) const
{
	OutQuests.Reset();

	if (!QuestDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestDataTable is not assigned."));
		return;
	}

	const TSet<FName>* SourceQuestIDs = nullptr;

	if (QuestState == EFTQuestStateType::Available)
	{
		SourceQuestIDs = &AvailableQuestIDs;
	}
	else if (QuestState == EFTQuestStateType::Active)
	{
		SourceQuestIDs = &ActiveQuestIDs;
	}
	else if (QuestState == EFTQuestStateType::Completed)
	{
		SourceQuestIDs = &CompletedQuestIDs;
	}

	if (!SourceQuestIDs)
	{
		return;
	}

	for (const FName& QuestID : *SourceQuestIDs)
	{
		const FTQuestStruct* Quest = QuestDataTable->FindRow<FTQuestStruct>(
			QuestID,
			TEXT("GetQuestListByState")
		);

		if (Quest)
		{
			OutQuests.Add(*Quest);
		}
	}
}

AFTHubStorage* UFTObjectiveSubsystem::GetHubStorage() const
{
	return HubStorage;
}

const FTQuestStruct* UFTObjectiveSubsystem::FindQuestByID(FName QuestID) const
{
	if (!QuestDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestDataTable is not assigned."));
		return nullptr;
	}

	return QuestDataTable->FindRow<FTQuestStruct>(QuestID, TEXT("FindQuestByID"));
}

bool UFTObjectiveSubsystem::IsQuestAvailable(FName QuestID) const
{
	return AvailableQuestIDs.Contains(QuestID);
}

bool UFTObjectiveSubsystem::AcceptQuest(FName QuestID)
{
	if (!AvailableQuestIDs.Contains(QuestID) || CompletedQuestIDs.Contains(QuestID))
	{
		return false;
	}

	AvailableQuestIDs.Remove(QuestID);
	ActiveQuestIDs.Add(QuestID);
	UE_LOG(LogTemp, Warning, TEXT("Quest Accepted: %s"), *QuestID.ToString());
	return true;
}

bool UFTObjectiveSubsystem::IsQuestActive(FName QuestID) const
{
	return ActiveQuestIDs.Contains(QuestID);
}

bool UFTObjectiveSubsystem::IsQuestCompleted(FName QuestID) const
{
	return CompletedQuestIDs.Contains(QuestID);
}

EFTQuestStateType UFTObjectiveSubsystem::GetQuestState(FName QuestID) const
{
	if (CompletedQuestIDs.Contains(QuestID))
	{
		return EFTQuestStateType::Completed;
	}

	if (ActiveQuestIDs.Contains(QuestID))
	{
		return EFTQuestStateType::Active;
	}

	if (AvailableQuestIDs.Contains(QuestID))
	{
		return EFTQuestStateType::Available;
	}

	return EFTQuestStateType::Locked;
}

void UFTObjectiveSubsystem::UnlockQuest(FName QuestID)
{
	if (QuestID.IsNone())
	{
		return;
	}

	if (CompletedQuestIDs.Contains(QuestID))
	{
		return;
	}

	if (ActiveQuestIDs.Contains(QuestID))
	{
		return;
	}

	AvailableQuestIDs.Add(QuestID);
}

int32 UFTObjectiveSubsystem::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
{
	const UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem
		? StorageSubsystem->GetCombinedItemCount(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID)
		: (PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0);
}

bool UFTObjectiveSubsystem::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count)
{
	UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem && StorageSubsystem->ConsumeCombinedItem(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID, Count);
}
