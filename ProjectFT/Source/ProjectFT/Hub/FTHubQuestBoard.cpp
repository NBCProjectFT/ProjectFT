#include "FTHubQuestBoard.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

AFTHubQuestBoard::AFTHubQuestBoard()
	: QuestDataTable(nullptr)
	, HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AFTHubQuestBoard::CanCompleteQuest(const FTQuestStruct& Quest) const
{
	if (!HubStorage)
	{
		return false;
	}

	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (HubStorage->GetStorageItemCount(RequiredItem.ItemID) < RequiredItem.Count)
		{
			return false;
		}
	}

	return true;
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

bool AFTHubQuestBoard::TryCompleteQuest(FName QuestID)
{
	const FTQuestStruct* Quest = FindQuestByID(QuestID);

	if (!Quest || !HubStorage || !CanCompleteQuest(*Quest))
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest Complete Failed: %s"), *QuestID.ToString());
		return false;
	}

	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		HubStorage->RemoveStorageItem(RequiredItem.ItemID, RequiredItem.Count);
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest->RewardItems)
	{
		HubStorage->AddStorageItem(RewardItem.ItemID, RewardItem.Count);
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

	TArray<FTQuestStruct*> QuestRows;
	QuestDataTable->GetAllRows<FTQuestStruct>(TEXT("GetQuestList"), QuestRows);

	for (const FTQuestStruct* Quest : QuestRows)
	{
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