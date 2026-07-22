#include "FTQuestViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"
#include "ProjectFT/UI/HubUI/FTQuestListObject.h"

namespace
{
	TArray<UObject*> ToRawObjectArray(const TArray<TObjectPtr<UObject>>& Objects)
	{
		TArray<UObject*> Result;
		Result.Reserve(Objects.Num());
		for (UObject* Object : Objects)
		{
			Result.Add(Object);
		}
		return Result;
	}
}

void UFTQuestViewModel::Initialize(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegate();

	ObjectiveSubsystem = InObjectiveSubsystem;
	PlayerInventory = InPlayerInventory;
	ClearSelection();

	BindInventoryDelegate();
	RefreshAll();
}

TArray<UObject*> UFTQuestViewModel::GetQuestObjects() const
{
	return ToRawObjectArray(QuestObjects);
}

TArray<UObject*> UFTQuestViewModel::GetRequiredItemObjects() const
{
	return ToRawObjectArray(RequiredItemObjects);
}

TArray<UObject*> UFTQuestViewModel::GetRewardItemObjects() const
{
	return ToRawObjectArray(RewardItemObjects);
}

UFTQuestListObject* UFTQuestViewModel::GetSelectedQuestObject() const
{
	return SelectedQuestObject;
}

int32 UFTQuestViewModel::GetActiveQuestCount() const
{
	if (!ObjectiveSubsystem)
	{
		return 0;
	}

	TArray<FTQuestStruct> AvailableQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Available, AvailableQuests);

	TArray<FTQuestStruct> ActiveQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Active, ActiveQuests);

	return AvailableQuests.Num() + ActiveQuests.Num();
}

int32 UFTQuestViewModel::GetCompletedQuestCount() const
{
	if (!ObjectiveSubsystem)
	{
		return 0;
	}

	TArray<FTQuestStruct> CompletedQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Completed, CompletedQuests);
	return CompletedQuests.Num();
}

EFTQuestStateType UFTQuestViewModel::GetSelectedQuestState() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && ObjectiveSubsystem
		? ObjectiveSubsystem->GetQuestState(Quest->QuestID)
		: EFTQuestStateType::Locked;
}

bool UFTQuestViewModel::IsActiveQuestTabSelected() const
{
	return CurrentQuestFilter == EFTQuestStateType::Active;
}

bool UFTQuestViewModel::IsCompletedQuestTabSelected() const
{
	return CurrentQuestFilter == EFTQuestStateType::Completed;
}

bool UFTQuestViewModel::HasSelectedQuestRequiredItems() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && !Quest->RequiredItems.IsEmpty();
}

bool UFTQuestViewModel::CanAcceptSelectedQuest() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && ObjectiveSubsystem && ObjectiveSubsystem->IsQuestAvailable(Quest->QuestID);
}

bool UFTQuestViewModel::CanCompleteSelectedQuest() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest
		&& ObjectiveSubsystem
		&& ObjectiveSubsystem->IsQuestActive(Quest->QuestID)
		&& ObjectiveSubsystem->CanCompleteQuest(*Quest, PlayerInventory);
}

bool UFTQuestViewModel::CanExecuteSelectedQuestAction() const
{
	return CanAcceptSelectedQuest() || CanCompleteSelectedQuest();
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
	UFTQuestListObject* NewSelectedQuestObject = Cast<UFTQuestListObject>(ItemObject);
	if (SelectedQuestObject == NewSelectedQuestObject)
	{
		return;
	}

	SelectedQuestObject = NewSelectedQuestObject;
	RefreshSelectedQuestItems();
	NotifyChanged();
}

bool UFTQuestViewModel::AcceptSelectedQuest()
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || !ObjectiveSubsystem || !ObjectiveSubsystem->AcceptQuest(Quest->QuestID))
	{
		return false;
	}

	CurrentQuestFilter = EFTQuestStateType::Active;
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

	bTransactionInProgress = true;
	const bool bCompleted = ObjectiveSubsystem->TryCompleteQuest(Quest->QuestID, PlayerInventory);
	bTransactionInProgress = false;
	if (!bCompleted)
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

bool UFTQuestViewModel::ExecuteSelectedQuestAction()
{
	if (CanAcceptSelectedQuest())
	{
		return AcceptSelectedQuest();
	}

	if (CanCompleteSelectedQuest())
	{
		return CompleteSelectedQuest();
	}

	return false;
}

void UFTQuestViewModel::HandleInventoryChanged()
{
	if (!bTransactionInProgress)
	{
		RefreshAll();
	}
}

void UFTQuestViewModel::RefreshQuestList()
{
	QuestObjects.Reset();

	if (!ObjectiveSubsystem)
	{
		return;
	}

	TArray<FTQuestStruct> Quests;
	if (CurrentQuestFilter == EFTQuestStateType::Active)
	{
		ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Available, Quests);

		TArray<FTQuestStruct> ActiveQuests;
		ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Active, ActiveQuests);
		Quests.Append(ActiveQuests);
	}
	else
	{
		ObjectiveSubsystem->GetQuestListByState(CurrentQuestFilter, Quests);
	}

	Quests.Sort([](const FTQuestStruct& Left, const FTQuestStruct& Right)
	{
		const int32 NameComparison = Left.QuestName.ToString().Compare(
			Right.QuestName.ToString(),
			ESearchCase::IgnoreCase);
		return NameComparison == 0
			? Left.QuestID.LexicalLess(Right.QuestID)
			: NameComparison < 0;
	});

	for (const FTQuestStruct& Quest : Quests)
	{
		UFTQuestListObject* QuestObject = NewObject<UFTQuestListObject>(this);
		QuestObject->Initialize(
			Quest,
			ObjectiveSubsystem->CanCompleteQuest(Quest, PlayerInventory),
			ObjectiveSubsystem->IsQuestActive(Quest.QuestID));
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
		ItemObject->SetShowSelectionCheckBox(false);
		RequiredItemObjects.Add(ItemObject);
	}

	for (const FTCraftIngredientStruct& RewardItem : Quest->RewardItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(RewardItem);
		ItemObject->SetShowSelectionCheckBox(false);
		RewardItemObjects.Add(ItemObject);
	}
}

void UFTQuestViewModel::RestoreSelection(const FName PreviousQuestID)
{
	SelectedQuestObject = nullptr;

	if (!PreviousQuestID.IsNone())
	{
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

	SelectedQuestObject = QuestObjects.IsEmpty()
		? nullptr
		: Cast<UFTQuestListObject>(QuestObjects[0]);
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

	BoundStorageInventory = ObjectiveSubsystem && ObjectiveSubsystem->GetHubStorage()
		? ObjectiveSubsystem->GetHubStorage()->GetStorageInventory()
		: nullptr;
	if (BoundStorageInventory && BoundStorageInventory != PlayerInventory)
	{
		BoundStorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
		BoundStorageInventory->OnInventoryChanged.AddDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
	}
}

void UFTQuestViewModel::UnbindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
	}
	if (BoundStorageInventory)
	{
		BoundStorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTQuestViewModel::HandleInventoryChanged);
		BoundStorageInventory = nullptr;
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
