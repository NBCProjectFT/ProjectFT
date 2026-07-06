#include "FTObjectiveSubsystem.h"

#include "../Message/FTGameplayTags.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

void UFTObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ItemPickedUp, this, &ThisClass::HandleItemPickedUpMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_RaidEscaped, this, &ThisClass::HandleRaidEscapedMessage));
}

void UFTObjectiveSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	for (FGameplayMessageListenerHandle& ListenerHandle : ObjectiveListenerHandles)
	{
		if (ListenerHandle.IsValid())
		{
			MessageSubsystem.UnregisterListener(ListenerHandle);
		}
	}
	ObjectiveListenerHandles.Reset();

	Super::Deinitialize();
}

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
	if (ItemId.IsNone() || !IsItemRequiredByActiveQuest(ItemId))
	{
		return;
	}

	int32& PickedUpCount = PickedUpItemCounts.FindOrAdd(ItemId);
	++PickedUpCount;

	if (RequiredItems.Contains(ItemId))
	{
		PickedUpRequiredItems.Add(ItemId);
	}

	for (const FName& QuestID : ActiveQuestIDs)
	{
		if (const FTQuestStruct* Quest = FindQuestByID(QuestID))
		{
			if (GetRequiredItemCountForQuest(*Quest, ItemId) > 0)
			{
				BroadcastQuestProgressChanged(QuestID);
			}
		}
	}
}

void UFTObjectiveSubsystem::NotifyEscapeReached()
{
}

float UFTObjectiveSubsystem::GetQuestProgress(FName QuestID) const
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);
	if (!Quest)
	{
		return 0.0f;
	}

	const int32 RequiredTotal = GetQuestRequiredTotal(*Quest);
	if (RequiredTotal <= 0)
	{
		return 1.0f;
	}

	return FMath::Clamp(static_cast<float>(GetQuestPickedUpTotal(*Quest)) / static_cast<float>(RequiredTotal), 0.0f, 1.0f);
}

FText UFTObjectiveSubsystem::GetQuestProgressText(FName QuestID) const
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);
	if (!Quest)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Printf(TEXT("%s %.0f%%"), *Quest->QuestName.ToString(), GetQuestProgress(QuestID) * 100.0f));
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

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.QuestId = QuestID;
	Payload.Value = 1.0f;
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ObjectiveCompleted, Payload);

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

	const FTQuestStruct* Quest = FindQuestByID(QuestID);
	if (!Quest)
	{
		return false;
	}

	AvailableQuestIDs.Remove(QuestID);
	ActiveQuestIDs.Add(QuestID);
	ActivateQuestProgress(*Quest);
	BroadcastQuestProgressChanged(QuestID);
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

void UFTObjectiveSubsystem::HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	AActor* InstigatorActor = Payload.InstigatorActor.Get();
	if (InstigatorActor)
	{
		LastProgressInventory = InstigatorActor->FindComponentByClass<UFTInventoryComponent>();
	}

	NotifyItemPickedUp(Payload.ItemId);
}

void UFTObjectiveSubsystem::HandleRaidEscapedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	CompleteTrackedQuestsOnEscape();
}

void UFTObjectiveSubsystem::ActivateQuestProgress(const FTQuestStruct& Quest)
{
	CurrentQuestId = Quest.QuestID;
	RequiredItems.Reset();
	PickedUpRequiredItems.Reset();

	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (!RequiredItem.ItemID.IsNone() && RequiredItem.Count > 0)
		{
			RequiredItems.Add(RequiredItem.ItemID);
		}
	}
}

void UFTObjectiveSubsystem::BroadcastQuestProgressChanged(FName QuestID) const
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.QuestId = QuestID;
	Payload.Value = GetQuestProgress(QuestID);
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ObjectiveProgressChanged, Payload);
}

void UFTObjectiveSubsystem::CompleteTrackedQuestsOnEscape()
{
	TArray<FName> QuestIDsToComplete;

	for (const FName& QuestID : ActiveQuestIDs)
	{
		const FTQuestStruct* Quest = FindQuestByID(QuestID);
		if (Quest && GetQuestProgress(QuestID) >= 1.0f)
		{
			QuestIDsToComplete.Add(QuestID);
		}
	}

	for (const FName& QuestID : QuestIDsToComplete)
	{
		TryCompleteQuest(QuestID, LastProgressInventory);
	}
}

bool UFTObjectiveSubsystem::IsItemRequiredByActiveQuest(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return false;
	}

	for (const FName& QuestID : ActiveQuestIDs)
	{
		if (const FTQuestStruct* Quest = FindQuestByID(QuestID))
		{
			if (GetRequiredItemCountForQuest(*Quest, ItemID) > 0)
			{
				return true;
			}
		}
	}

	return false;
}

int32 UFTObjectiveSubsystem::GetRequiredItemCountForQuest(const FTQuestStruct& Quest, FName ItemID) const
{
	int32 RequiredCount = 0;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (RequiredItem.ItemID == ItemID)
		{
			RequiredCount += FMath::Max(0, RequiredItem.Count);
		}
	}

	return RequiredCount;
}

int32 UFTObjectiveSubsystem::GetPickedUpItemCount(FName ItemID) const
{
	if (const int32* Count = PickedUpItemCounts.Find(ItemID))
	{
		return *Count;
	}

	return 0;
}

int32 UFTObjectiveSubsystem::GetQuestRequiredTotal(const FTQuestStruct& Quest) const
{
	int32 RequiredTotal = 0;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		RequiredTotal += FMath::Max(0, RequiredItem.Count);
	}

	return RequiredTotal;
}

int32 UFTObjectiveSubsystem::GetQuestPickedUpTotal(const FTQuestStruct& Quest) const
{
	int32 PickedUpTotal = 0;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		const int32 RequiredCount = FMath::Max(0, RequiredItem.Count);
		PickedUpTotal += FMath::Min(GetPickedUpItemCount(RequiredItem.ItemID), RequiredCount);
	}

	return PickedUpTotal;
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
