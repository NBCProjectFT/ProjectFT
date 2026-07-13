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

FText UFTQuestViewModel::GetSelectedQuestSenderText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest)
	{
		return FText::GetEmpty();
	}

	return Quest->SenderName.IsEmpty()
		? FText::FromString(TEXT("Hub Mail"))
		: Quest->SenderName;
}

FText UFTQuestViewModel::GetSelectedQuestDescriptionText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest)
	{
		return FText::GetEmpty();
	}

	FString Description = Quest->Description.ToString().TrimStartAndEnd();

	Description.ReplaceInline(TEXT(". "), TEXT(".\n\n"));
	Description.ReplaceInline(TEXT("! "), TEXT("!\n\n"));
	Description.ReplaceInline(TEXT("? "), TEXT("?\n\n"));

	return FText::FromString(Description);
}

FText UFTQuestViewModel::GetSelectedQuestObjectiveLinesText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || Quest->ObjectiveLines.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FString ObjectiveText;
	for (const FText& ObjectiveLine : Quest->ObjectiveLines)
	{
		if (ObjectiveLine.IsEmpty())
		{
			continue;
		}

		if (!ObjectiveText.IsEmpty())
		{
			ObjectiveText += LINE_TERMINATOR;
		}

		ObjectiveText += FString::Printf(TEXT("- %s"), *ObjectiveLine.ToString());
	}

	return FText::FromString(ObjectiveText);
}

FText UFTQuestViewModel::GetSelectedQuestCurrencyRewardText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || Quest->CurrencyReward <= 0)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Printf(TEXT("%d 코인"), Quest->CurrencyReward));
}

FText UFTQuestViewModel::GetSelectedQuestActionText() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	if (!Quest || !ObjectiveSubsystem)
	{
		return FText::FromString(TEXT("선택"));
	}

	if (ObjectiveSubsystem->IsQuestAvailable(Quest->QuestID))
	{
		return FText::FromString(TEXT("수락"));
	}

	if (ObjectiveSubsystem->IsQuestActive(Quest->QuestID))
	{
		return FText::FromString(TEXT("완료"));
	}

	return FText::FromString(TEXT("확인"));
}

FText UFTQuestViewModel::GetActiveQuestCountText() const
{
	if (!ObjectiveSubsystem)
	{
		return FText::FromString(TEXT("0"));
	}

	TArray<FTQuestStruct> AvailableQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Available, AvailableQuests);

	TArray<FTQuestStruct> ActiveQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Active, ActiveQuests);

	return FText::AsNumber(AvailableQuests.Num() + ActiveQuests.Num());
}

FText UFTQuestViewModel::GetCompletedQuestCountText() const
{
	if (!ObjectiveSubsystem)
	{
		return FText::FromString(TEXT("0"));
	}

	TArray<FTQuestStruct> CompletedQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Completed, CompletedQuests);
	return FText::AsNumber(CompletedQuests.Num());
}

bool UFTQuestViewModel::IsActiveQuestTabSelected() const
{
	return CurrentQuestFilter == EFTQuestStateType::Active;
}

bool UFTQuestViewModel::IsCompletedQuestTabSelected() const
{
	return CurrentQuestFilter == EFTQuestStateType::Completed;
}

bool UFTQuestViewModel::CanAcceptSelectedQuest() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && ObjectiveSubsystem && ObjectiveSubsystem->IsQuestAvailable(Quest->QuestID);
}

bool UFTQuestViewModel::HasSelectedQuestRequiredItems() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && !Quest->RequiredItems.IsEmpty();
}

bool UFTQuestViewModel::CanCompleteSelectedQuest() const
{
	const FTQuestStruct* Quest = GetSelectedQuest();
	return Quest && ObjectiveSubsystem && ObjectiveSubsystem->IsQuestActive(Quest->QuestID) && SelectedQuestObject->CanComplete();
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
