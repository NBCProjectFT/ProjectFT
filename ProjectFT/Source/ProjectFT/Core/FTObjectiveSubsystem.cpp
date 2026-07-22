#include "FTObjectiveSubsystem.h"

#include "../Message/FTGameplayTags.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Core/FTSaveSubsystem.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityResponsePayloadStruct.h"

void UFTObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_RaidEscaped, this, &ThisClass::HandleRaidEscapedMessage));

	// FFTMessagePayloadStruct 기반의 횟수형 마트 사건.
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ItemConsumed, this, &ThisClass::HandleQuestMessage));
	// 제작·구매는 한 번의 요청에서 여러 개를 얻을 수 있으므로 Payload.Value를 진행 수량으로 사용한다.
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_CraftCompleted, this, &ThisClass::HandleCountedItemQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ShopPurchased, this, &ThisClass::HandleCountedItemQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ShopSold, this, &ThisClass::HandleCountedItemQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_HubComputerAccessed, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ShelfDamaged, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_ShelfDestroyed, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_StealCompleted, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_RaidStarted, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_RaidFailed, this, &ThisClass::HandleQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_PlayerDead, this, &ThisClass::HandleQuestMessage));

	// NPC 신고 및 체포 사건.
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_NPCDetectedPlayer, this, &ThisClass::HandleNPCQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_NPCReportStarted, this, &ThisClass::HandleNPCQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_NPCReportCompleted, this, &ThisClass::HandleNPCQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityCalled, this, &ThisClass::HandleNPCQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityTargetCaptured, this, &ThisClass::HandleNPCQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityTargetEscaped, this, &ThisClass::HandleNPCQuestMessage));

	// 추격 상태의 이산 사건. 매 프레임성 GaugeChanged는 의도적으로 구독하지 않는다.
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityTargetSeen, this, &ThisClass::HandleSecurityChaseQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityTargetLost, this, &ThisClass::HandleSecurityChaseQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityChaseEnded, this, &ThisClass::HandleSecurityChaseQuestMessage));

	// 경비실 배치/복귀 사건.
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityDeployed, this, &ThisClass::HandleSecurityResponseQuestMessage));
	ObjectiveListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityReturnedToRoom, this, &ThisClass::HandleSecurityResponseQuestMessage));
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
	const FTQuestStruct* Quest = FindQuestByID(CurrentQuestId);
	if (!Quest || !AreQuestEventConditionsCompleted(*Quest))
	{
		return false;
	}

	UFTInventoryComponent* PlayerInventory = ResolvePlayerInventory();
	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		if (GetCombinedItemCount(PlayerInventory, RequiredItem.ItemID) < RequiredItem.Count)
		{
			return false;
		}
	}

	return true;
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

	const int32 RequiredTotal = GetQuestRequiredTotal(*Quest) + GetQuestEventRequiredTotal(*Quest);
	if (RequiredTotal <= 0)
	{
		return 1.0f;
	}

	const int32 ProgressTotal = GetQuestItemProgressTotal(*Quest, ResolvePlayerInventory()) + GetQuestEventProgressTotal(*Quest);
	return FMath::Clamp(static_cast<float>(ProgressTotal) / static_cast<float>(RequiredTotal), 0.0f, 1.0f);
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

FText UFTObjectiveSubsystem::GetQuestObjectiveProgressText(FName QuestID) const
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);
	if (!Quest)
	{
		return FText::GetEmpty();
	}

	TArray<FString> DetailLines;
	int32 ObjectiveLineIndex = 0;

	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		const int32 RequiredCount = FMath::Max(0, RequiredItem.Count);
		if (RequiredItem.ItemID.IsNone() || RequiredCount <= 0)
		{
			++ObjectiveLineIndex;
			continue;
		}

		const FString Label = Quest->ObjectiveLines.IsValidIndex(ObjectiveLineIndex)
			? Quest->ObjectiveLines[ObjectiveLineIndex].ToString()
			: RequiredItem.ItemID.ToString();
		const int32 CurrentCount = FMath::Min(
			GetCombinedItemCount(ResolvePlayerInventory(), RequiredItem.ItemID),
			RequiredCount);
		DetailLines.Add(FString::Printf(TEXT("%s  %d / %d"), *Label, CurrentCount, RequiredCount));
		++ObjectiveLineIndex;
	}

	const TArray<int32>* EventProgressValues = EventConditionProgressByQuest.Find(QuestID);
	for (int32 ConditionIndex = 0; ConditionIndex < Quest->EventConditions.Num(); ++ConditionIndex)
	{
		const FFTQuestConditionStruct& Condition = Quest->EventConditions[ConditionIndex];
		if (!Condition.IsValid())
		{
			++ObjectiveLineIndex;
			continue;
		}

		const FString Label = Quest->ObjectiveLines.IsValidIndex(ObjectiveLineIndex)
			? Quest->ObjectiveLines[ObjectiveLineIndex].ToString()
			: Condition.EventTag.ToString();
		const int32 CurrentCount = EventProgressValues && EventProgressValues->IsValidIndex(ConditionIndex)
			? FMath::Min((*EventProgressValues)[ConditionIndex], Condition.RequiredCount)
			: 0;
		DetailLines.Add(FString::Printf(TEXT("%s  %d / %d"), *Label, CurrentCount, Condition.RequiredCount));
		++ObjectiveLineIndex;
	}

	// 아직 추적 조건에 연결하지 않은 안내 문구도 누락하지 않고 표시한다.
	for (; ObjectiveLineIndex < Quest->ObjectiveLines.Num(); ++ObjectiveLineIndex)
	{
		DetailLines.Add(Quest->ObjectiveLines[ObjectiveLineIndex].ToString());
	}

	return FText::FromString(FString::Join(DetailLines, LINE_TERMINATOR));
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
	if (!AreQuestEventConditionsCompleted(Quest))
	{
		return false;
	}

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

	return CanGrantQuestRewards(Quest, PlayerInventory);
}

bool UFTObjectiveSubsystem::TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory)
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);

	if (CompletedQuestIDs.Contains(QuestID))
	{
		return false;
	}

	// 보상 지급은 터미널에서 수락된 활성 퀘스트를 명시적으로 제출할 때만 허용한다.
	if (!Quest || !PlayerInventory || !ActiveQuestIDs.Contains(QuestID) || !CanCompleteQuest(*Quest, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest Complete Failed: %s"), *QuestID.ToString());
		return false;
	}

	struct FConsumedQuestItem
	{
		FName ItemID = NAME_None;
		int32 PlayerCount = 0;
		int32 StorageCount = 0;
	};

	UFTInventoryComponent* StorageInventory = HubStorage ? HubStorage->GetStorageInventory() : nullptr;
	UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;
	TArray<FConsumedQuestItem> ConsumedItems;
	TArray<FTCraftIngredientStruct> GrantedRewardItems;

	auto RollbackQuestTransaction = [&]()
	{
		for (const FTCraftIngredientStruct& GrantedReward : GrantedRewardItems)
		{
			PlayerInventory->RemoveItem(GrantedReward.ItemID, GrantedReward.Count);
		}

		for (const FConsumedQuestItem& ConsumedItem : ConsumedItems)
		{
			if (ConsumedItem.PlayerCount > 0)
			{
				PlayerInventory->AddItem(ConsumedItem.ItemID, ConsumedItem.PlayerCount);
			}
			if (ConsumedItem.StorageCount > 0 && StorageSubsystem && StorageInventory)
			{
				StorageSubsystem->AddStorageItem(StorageInventory, ConsumedItem.ItemID, ConsumedItem.StorageCount);
			}
		}
	};

	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		FConsumedQuestItem ConsumedItem;
		ConsumedItem.ItemID = RequiredItem.ItemID;
		ConsumedItem.PlayerCount = FMath::Min(
			PlayerInventory->GetItemQuantity(RequiredItem.ItemID),
			RequiredItem.Count);
		ConsumedItem.StorageCount = RequiredItem.Count - ConsumedItem.PlayerCount;

		if (!ConsumeCombinedItem(PlayerInventory, RequiredItem.ItemID, RequiredItem.Count))
		{
			RollbackQuestTransaction();
			UE_LOG(LogTemp, Warning, TEXT("Quest Complete Failed: %s"), *QuestID.ToString());
			return false;
		}
		ConsumedItems.Add(ConsumedItem);
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest->RewardItems)
	{
		if (!PlayerInventory->AddItem(RewardItem.ItemID, RewardItem.Count))
		{
			RollbackQuestTransaction();
			UE_LOG(LogTemp, Warning, TEXT("Quest Reward Failed: %s"), *QuestID.ToString());
			return false;
		}
		GrantedRewardItems.Add(RewardItem);
	}

	if (Quest->CurrencyReward > 0)
	{
		FName CurrencyItemID = TEXT("ID_Common_Coin");
		if (UFTShopSubsystem* ShopSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFTShopSubsystem>() : nullptr)
		{
			CurrencyItemID = ShopSubsystem->GetCurrencyItemID();
		}

		if (CurrencyItemID.IsNone() || !PlayerInventory->AddItem(CurrencyItemID, Quest->CurrencyReward))
		{
			RollbackQuestTransaction();
			UE_LOG(LogTemp, Warning, TEXT("Quest Currency Reward Failed: %s / Amount %d"), *QuestID.ToString(), Quest->CurrencyReward);
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

void UFTObjectiveSubsystem::BuildQuestSaveData(FFTQuestSaveData& OutSaveData) const
{
	OutSaveData = FFTQuestSaveData();
	OutSaveData.bHasQuestData = true;
	OutSaveData.AvailableQuestIDs = AvailableQuestIDs.Array();
	OutSaveData.ActiveQuestIDs = ActiveQuestIDs.Array();
	OutSaveData.CompletedQuestIDs = CompletedQuestIDs.Array();
	OutSaveData.CurrentQuestID = CurrentQuestId;

	auto SortQuestIDs = [](TArray<FName>& QuestIDs)
	{
		QuestIDs.Sort([](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
	};

	SortQuestIDs(OutSaveData.AvailableQuestIDs);
	SortQuestIDs(OutSaveData.ActiveQuestIDs);
	SortQuestIDs(OutSaveData.CompletedQuestIDs);

	for (const FName& QuestID : OutSaveData.ActiveQuestIDs)
	{
		FFTQuestEventProgressSaveData ProgressSaveData;
		ProgressSaveData.QuestID = QuestID;
		if (const TArray<int32>* ProgressValues = EventConditionProgressByQuest.Find(QuestID))
		{
			ProgressSaveData.EventConditionProgress = *ProgressValues;
		}
		OutSaveData.EventProgressByQuest.Add(MoveTemp(ProgressSaveData));
	}
}

void UFTObjectiveSubsystem::RestoreQuestSaveData(const FFTQuestSaveData& SaveData)
{
	if (!SaveData.bHasQuestData)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest save restore skipped: no quest snapshot in save data."));
		return;
	}

	const bool bHasSavedQuestState = !SaveData.AvailableQuestIDs.IsEmpty()
		|| !SaveData.ActiveQuestIDs.IsEmpty()
		|| !SaveData.CompletedQuestIDs.IsEmpty();
	if (!bHasSavedQuestState)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Quest save restore skipped: snapshot is marked valid but contains no quest state. Keeping configured initial quests."));
		return;
	}

	if (!QuestDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Quest save restore failed: QuestDataTable is not assigned."));
		return;
	}

	auto IsValidQuestID = [this](const FName QuestID)
	{
		return !QuestID.IsNone()
			&& QuestDataTable->FindRow<FTQuestStruct>(QuestID, TEXT("RestoreQuestSaveData"), false) != nullptr;
	};

	AvailableQuestIDs.Reset();
	ActiveQuestIDs.Reset();
	CompletedQuestIDs.Reset();
	EventConditionProgressByQuest.Reset();
	CurrentQuestId = NAME_None;
	RequiredItems.Reset();

	// 완료 상태가 가장 높은 우선순위를 가진다.
	for (const FName& QuestID : SaveData.CompletedQuestIDs)
	{
		if (IsValidQuestID(QuestID))
		{
			CompletedQuestIDs.Add(QuestID);
		}
	}

	for (const FName& QuestID : SaveData.ActiveQuestIDs)
	{
		if (IsValidQuestID(QuestID) && !CompletedQuestIDs.Contains(QuestID))
		{
			ActiveQuestIDs.Add(QuestID);
		}
	}

	for (const FName& QuestID : SaveData.AvailableQuestIDs)
	{
		if (IsValidQuestID(QuestID)
			&& !CompletedQuestIDs.Contains(QuestID)
			&& !ActiveQuestIDs.Contains(QuestID))
		{
			AvailableQuestIDs.Add(QuestID);
		}
	}

	TMap<FName, const FFTQuestEventProgressSaveData*> SavedProgressByQuest;
	for (const FFTQuestEventProgressSaveData& ProgressSaveData : SaveData.EventProgressByQuest)
	{
		if (ActiveQuestIDs.Contains(ProgressSaveData.QuestID))
		{
			SavedProgressByQuest.FindOrAdd(ProgressSaveData.QuestID) = &ProgressSaveData;
		}
	}

	for (const FName& QuestID : ActiveQuestIDs)
	{
		const FTQuestStruct* Quest = FindQuestByID(QuestID);
		if (!Quest)
		{
			continue;
		}

		TArray<int32>& RestoredProgress = EventConditionProgressByQuest.FindOrAdd(QuestID);
		RestoredProgress.Init(0, Quest->EventConditions.Num());

		const FFTQuestEventProgressSaveData* const* SavedProgress = SavedProgressByQuest.Find(QuestID);
		if (!SavedProgress || !*SavedProgress)
		{
			continue;
		}

		const int32 CopyCount = FMath::Min(
			RestoredProgress.Num(),
			(*SavedProgress)->EventConditionProgress.Num());
		for (int32 ConditionIndex = 0; ConditionIndex < CopyCount; ++ConditionIndex)
		{
			const int32 RequiredCount = FMath::Max(
				0,
				Quest->EventConditions[ConditionIndex].RequiredCount);
			RestoredProgress[ConditionIndex] = FMath::Clamp(
				(*SavedProgress)->EventConditionProgress[ConditionIndex],
				0,
				RequiredCount);
		}
	}

	if (ActiveQuestIDs.Contains(SaveData.CurrentQuestID))
	{
		CurrentQuestId = SaveData.CurrentQuestID;
	}
	else if (!ActiveQuestIDs.IsEmpty())
	{
		TArray<FName> SortedActiveQuestIDs = ActiveQuestIDs.Array();
		SortedActiveQuestIDs.Sort([](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
		CurrentQuestId = SortedActiveQuestIDs[0];
	}

	if (!CurrentQuestId.IsNone())
	{
		if (const FTQuestStruct* CurrentQuest = FindQuestByID(CurrentQuestId))
		{
			for (const FTCraftIngredientStruct& RequiredItem : CurrentQuest->RequiredItems)
			{
				if (!RequiredItem.ItemID.IsNone() && RequiredItem.Count > 0)
				{
					RequiredItems.Add(RequiredItem.ItemID);
				}
			}
		}
	}

	if (ActiveQuestIDs.IsEmpty())
	{
		// 활성 퀘스트가 없어도 이미 열린 HUD가 목록을 비울 수 있도록 한 번 알린다.
		BroadcastQuestProgressChanged(NAME_None);
	}
	else
	{
		for (const FName& QuestID : ActiveQuestIDs)
		{
			BroadcastQuestProgressChanged(QuestID);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Quest save restored: Available=%d Active=%d Completed=%d"),
		AvailableQuestIDs.Num(),
		ActiveQuestIDs.Num(),
		CompletedQuestIDs.Num());
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

void UFTObjectiveSubsystem::HandleQuestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	ApplyQuestEvent(Channel, Payload.ItemId);
}

void UFTObjectiveSubsystem::HandleCountedItemQuestMessage(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	// 성공 이벤트의 Value는 항상 양수 정수 수량으로 발행한다. 잘못된 외부 메시지는 최소 1회로 보정한다.
	const int32 ItemCount = FMath::Max(1, FMath::RoundToInt(Payload.Value));
	ApplyQuestEvent(Channel, Payload.ItemId, ItemCount);
}

void UFTObjectiveSubsystem::HandleNPCQuestMessage(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	ApplyQuestEvent(Channel);
}

void UFTObjectiveSubsystem::HandleSecurityChaseQuestMessage(
	FGameplayTag Channel,
	const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	ApplyQuestEvent(Channel);
}

void UFTObjectiveSubsystem::HandleSecurityResponseQuestMessage(
	FGameplayTag Channel,
	const FFTSecurityResponsePayloadStruct& Payload)
{
	ApplyQuestEvent(Channel);
}

void UFTObjectiveSubsystem::HandleRaidEscapedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	// 탈출은 조건 진행만 기록한다. 완료 처리와 보상 지급은 터미널 제출 시 TryCompleteQuest에서 수행한다.
	ApplyQuestEvent(Channel);
}

void UFTObjectiveSubsystem::ApplyQuestEvent(FGameplayTag EventTag, FName ItemID, int32 Count)
{
	if (!EventTag.IsValid() || Count <= 0)
	{
		return;
	}

	for (const FName& QuestID : ActiveQuestIDs)
	{
		const FTQuestStruct* Quest = FindQuestByID(QuestID);
		if (!Quest || Quest->EventConditions.IsEmpty())
		{
			continue;
		}

		TArray<int32>& ProgressValues = EventConditionProgressByQuest.FindOrAdd(QuestID);
		if (ProgressValues.Num() < Quest->EventConditions.Num())
		{
			ProgressValues.AddZeroed(Quest->EventConditions.Num() - ProgressValues.Num());
		}
		else if (ProgressValues.Num() > Quest->EventConditions.Num())
		{
			ProgressValues.SetNum(Quest->EventConditions.Num());
		}
		bool bProgressChanged = false;

		for (int32 ConditionIndex = 0; ConditionIndex < Quest->EventConditions.Num(); ++ConditionIndex)
		{
			const FFTQuestConditionStruct& Condition = Quest->EventConditions[ConditionIndex];
			if (!Condition.IsValid() || Condition.EventTag != EventTag)
			{
				continue;
			}

			if (!Condition.ItemID.IsNone() && Condition.ItemID != ItemID)
			{
				continue;
			}

			const int32 PreviousProgress = ProgressValues[ConditionIndex];
			ProgressValues[ConditionIndex] = FMath::Min(PreviousProgress + Count, Condition.RequiredCount);
			bProgressChanged |= ProgressValues[ConditionIndex] != PreviousProgress;
		}

		if (bProgressChanged)
		{
			BroadcastQuestProgressChanged(QuestID);
		}
	}
}

void UFTObjectiveSubsystem::ActivateQuestProgress(const FTQuestStruct& Quest)
{
	CurrentQuestId = Quest.QuestID;
	RequiredItems.Reset();

	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (!RequiredItem.ItemID.IsNone() && RequiredItem.Count > 0)
		{
			RequiredItems.Add(RequiredItem.ItemID);
		}
	}

	TArray<int32>& EventProgress = EventConditionProgressByQuest.FindOrAdd(Quest.QuestID);
	EventProgress.Init(0, Quest.EventConditions.Num());
}

void UFTObjectiveSubsystem::BroadcastQuestProgressChanged(FName QuestID)
{
	OnQuestStateChanged.Broadcast(QuestID);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.QuestId = QuestID;
	Payload.Value = GetQuestProgress(QuestID);
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ObjectiveProgressChanged, Payload);
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

int32 UFTObjectiveSubsystem::GetQuestItemProgressTotal(
	const FTQuestStruct& Quest,
	UFTInventoryComponent* PlayerInventory) const
{
	int32 ItemProgressTotal = 0;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		const int32 RequiredCount = FMath::Max(0, RequiredItem.Count);
		if (!RequiredItem.ItemID.IsNone() && RequiredCount > 0)
		{
			ItemProgressTotal += FMath::Min(
				GetCombinedItemCount(PlayerInventory, RequiredItem.ItemID),
				RequiredCount);
		}
	}

	return ItemProgressTotal;
}

int32 UFTObjectiveSubsystem::GetQuestEventRequiredTotal(const FTQuestStruct& Quest) const
{
	int32 RequiredTotal = 0;
	for (const FFTQuestConditionStruct& Condition : Quest.EventConditions)
	{
		if (Condition.IsValid())
		{
			RequiredTotal += Condition.RequiredCount;
		}
	}
	return RequiredTotal;
}

int32 UFTObjectiveSubsystem::GetQuestEventProgressTotal(const FTQuestStruct& Quest) const
{
	const TArray<int32>* ProgressValues = EventConditionProgressByQuest.Find(Quest.QuestID);
	if (!ProgressValues)
	{
		return 0;
	}

	int32 ProgressTotal = 0;
	for (int32 ConditionIndex = 0; ConditionIndex < Quest.EventConditions.Num(); ++ConditionIndex)
	{
		const FFTQuestConditionStruct& Condition = Quest.EventConditions[ConditionIndex];
		if (Condition.IsValid() && ProgressValues->IsValidIndex(ConditionIndex))
		{
			ProgressTotal += FMath::Min((*ProgressValues)[ConditionIndex], Condition.RequiredCount);
		}
	}
	return ProgressTotal;
}

bool UFTObjectiveSubsystem::AreQuestEventConditionsCompleted(const FTQuestStruct& Quest) const
{
	return GetQuestEventProgressTotal(Quest) >= GetQuestEventRequiredTotal(Quest);
}

bool UFTObjectiveSubsystem::CanGrantQuestRewards(
	const FTQuestStruct& Quest,
	UFTInventoryComponent* PlayerInventory) const
{
	if (!PlayerInventory)
	{
		return false;
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest.RewardItems)
	{
		if (!RewardItem.ItemID.IsNone()
			&& RewardItem.Count > 0
			&& !PlayerInventory->CanAddItem(RewardItem.ItemID, RewardItem.Count))
		{
			return false;
		}
	}

	if (Quest.CurrencyReward > 0)
	{
		FName CurrencyItemID = TEXT("ID_Common_Coin");
		if (const UFTShopSubsystem* ShopSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFTShopSubsystem>()
			: nullptr)
		{
			CurrencyItemID = ShopSubsystem->GetCurrencyItemID();
		}

		if (CurrencyItemID.IsNone() || !PlayerInventory->CanAddItem(CurrencyItemID, Quest.CurrencyReward))
		{
			return false;
		}
	}

	return true;
}

int32 UFTObjectiveSubsystem::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
{
	const int32 PlayerItemCount = PlayerInventory
		? PlayerInventory->GetItemQuantity(ItemID)
		: 0;

	// HubStorage is a level actor and becomes invalid after leaving the Hub. While outside
	// the Hub, use the read-only storage snapshot captured immediately before level travel.
	if (!IsValid(HubStorage))
	{
		const UFTSaveSubsystem* SaveSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFTSaveSubsystem>()
			: nullptr;
		return PlayerItemCount + (SaveSubsystem ? SaveSubsystem->GetStorageSnapshotItemCount(ItemID) : 0);
	}

	const UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem
		? StorageSubsystem->GetCombinedItemCount(PlayerInventory, HubStorage->GetStorageInventory(), ItemID)
		: PlayerItemCount;
}

bool UFTObjectiveSubsystem::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count)
{
	UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem && StorageSubsystem->ConsumeCombinedItem(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID, Count);
}

UFTInventoryComponent* UFTObjectiveSubsystem::ResolvePlayerInventory() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
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
