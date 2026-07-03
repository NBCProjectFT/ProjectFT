#include "FTQuestViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"
#include "ProjectFT/UI/HubUI/FTQuestListObject.h"

void UFTQuestViewModel::Initialize(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegate();

	ObjectiveSubsystem = InObjectiveSubsystem;
	PlayerInventory = InPlayerInventory;
	ClearSelection();

	BindInventoryDelegate();
	RefreshAll();
}

const TArray<TObjectPtr<UObject>>& UFTQuestViewModel::GetQuestObjects() const
{
	return QuestObjects;
}

const TArray<TObjectPtr<UObject>>& UFTQuestViewModel::GetRequiredItemObjects() const
{
	return RequiredItemObjects;
}

const TArray<TObjectPtr<UObject>>& UFTQuestViewModel::GetRewardItemObjects() const
{
	return RewardItemObjects;
}

UFTQuestListObject* UFTQuestViewModel::GetSelectedQuestObject() const
{
	return SelectedQuestObject;
}

FText UFTQuestViewModel::GetSelectedQuestNameText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest ? Quest->QuestName : FText::FromString(TEXT("Select Quest"));
}

FText UFTQuestViewModel::GetSelectedQuestDescriptionText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest ? Quest->Description : FText::GetEmpty();
}

bool UFTQuestViewModel::CanAcceptSelectedQuest() const
{
	return SelectedQuestObject && CurrentQuestFilter == EFTQuestStateType::Available;
}

bool UFTQuestViewModel::CanCompleteSelectedQuest() const
{
	return SelectedQuestObject && CurrentQuestFilter == EFTQuestStateType::Active && SelectedQuestObject->CanComplete();
}

void UFTQuestViewModel::RefreshAll()
{
	const FName PreviousQuestID = SelectedQuestObject ? SelectedQuestObject->GetQuest().QuestID : NAME_None;

	RefreshQuestList();
	RestoreSelection(PreviousQuestID);
	RefreshSelectedQuestItems();
	NotifyChanged();
}

void UFTQuestViewModel::SetQuestFilter(const EFTQuestStateType NewQuestFilter)
{
	if (CurrentQuestFilter == NewQuestFilter)
	{
		return;
	}

	CurrentQuestFilter = NewQuestFilter;
	ClearSelection();
	RefreshAll();
}

void UFTQuestViewModel::SelectQuestObject(UObject* ItemObject)
{
	SelectedQuestObject = Cast<UFTQuestListObject>(ItemObject);
	RefreshSelectedQuestItems();
	NotifyChanged();
}

bool UFTQuestViewModel::AcceptSelectedQuest()
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || !ObjectiveSubsystem)
	{
		return false;
	}

	if (!ObjectiveSubsystem->AcceptQuest(Quest->QuestID))
	{
		return false;
	}

	CurrentQuestFilter = EFTQuestStateType::Active;
	ClearSelection();
	RefreshAll();
	return true;
}

bool UFTQuestViewModel::CompleteSelectedQuest()
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || !ObjectiveSubsystem)
	{
		return false;
	}

	if (!ObjectiveSubsystem->TryCompleteQuest(Quest->QuestID, PlayerInventory))
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

void UFTQuestViewModel::HandleInventoryChanged()
{
	RefreshAll();
}

void UFTQuestViewModel::RefreshQuestList()
{
	QuestObjects.Reset();

	if (!ObjectiveSubsystem)
	{
		return;
	}

	TArray<FTQuestStruct> Quests;
	ObjectiveSubsystem->GetQuestListByState(CurrentQuestFilter, Quests);

	for (const FTQuestStruct& Quest : Quests)
	{
		UFTQuestListObject* QuestObject = NewObject<UFTQuestListObject>(this);
		QuestObject->Initialize(Quest, ObjectiveSubsystem->CanCompleteQuest(Quest, PlayerInventory));
		QuestObjects.Add(QuestObject);
	}
}

void UFTQuestViewModel::RefreshSelectedQuestItems()
{
	RequiredItemObjects.Reset();
	RewardItemObjects.Reset();

	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest)
	{
		return;
	}

	for (const FTCraftIngredientStruct& RequiredItem : Quest->RequiredItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(RequiredItem);
		RequiredItemObjects.Add(ItemObject);
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest->RewardItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(RewardItem);
		RewardItemObjects.Add(ItemObject);
	}
}

void UFTQuestViewModel::RestoreSelection(const FName PreviousQuestID)
{
	SelectedQuestObject = nullptr;

	if (PreviousQuestID.IsNone())
	{
		return;
	}

	for (UObject* ItemObject : QuestObjects)
	{
		UFTQuestListObject* QuestObject = Cast<UFTQuestListObject>(ItemObject);
		if (QuestObject && QuestObject->GetQuest().QuestID == PreviousQuestID)
		{
			SelectedQuestObject = QuestObject;
			return;
		}
	}
}

void UFTQuestViewModel::ClearSelection()
{
	SelectedQuestObject = nullptr;
	RequiredItemObjects.Reset();
	RewardItemObjects.Reset();
}

void UFTQuestViewModel::BindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
	}
}

void UFTQuestViewModel::UnbindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
	}
}

const FTQuestStruct* UFTQuestViewModel::GetSelectedQuest() const
{
	return SelectedQuestObject ? &SelectedQuestObject->GetQuest() : nullptr;
}

void UFTQuestViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
