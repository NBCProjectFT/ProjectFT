#include "FTHubQuestPanelWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "FTItemTileListObject.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubQuestBoard.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTHubQuestPanelWidget::InitializeQuestPanel(AFTHubQuestBoard* InQuestBoard, UFTInventoryComponent* InPlayerInventory)
{
	QuestBoard = InQuestBoard;
	PlayerInventory = InPlayerInventory;
	SelectedQuest = nullptr;
	RefreshQuestList();
	UpdateSelectedQuestDetails();
}

void UFTHubQuestPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_Quests)
	{
		LV_Quests->OnItemClicked().RemoveAll(this);
		LV_Quests->OnItemClicked().AddUObject(this, &UFTHubQuestPanelWidget::HandleQuestClicked);
	}

	if (BTN_AvailableQuestTab)
	{
		BTN_AvailableQuestTab->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleAvailableQuestTabClicked);
		BTN_AvailableQuestTab->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleAvailableQuestTabClicked);
	}

	if (BTN_ActiveQuestTab)
	{
		BTN_ActiveQuestTab->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleActiveQuestTabClicked);
		BTN_ActiveQuestTab->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleActiveQuestTabClicked);
	}

	if (BTN_CompletedQuestTab)
	{
		BTN_CompletedQuestTab->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleCompletedQuestTabClicked);
		BTN_CompletedQuestTab->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleCompletedQuestTabClicked);
	}

	if (BTN_CompleteQuest)
	{
		BTN_CompleteQuest->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleCompleteQuestClicked);
		BTN_CompleteQuest->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleCompleteQuestClicked);
	}

	if (BTN_AcceptQuest)
	{
		BTN_AcceptQuest->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleAcceptQuestClicked);
		BTN_AcceptQuest->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleAcceptQuestClicked);
	}

	RefreshQuestList();
	UpdateSelectedQuestDetails();
}

void UFTHubQuestPanelWidget::RefreshQuestList()
{
	if (!LV_Quests)
	{
		return;
	}

	const FName SelectedQuestID = SelectedQuest
		? SelectedQuest->GetQuest().QuestID
		: NAME_None;

	SelectedQuest = nullptr;
	LV_Quests->ClearListItems();

	if (!QuestBoard)
	{
		return;
	}

	TArray<FTQuestStruct> Quests;
	QuestBoard->GetQuestListByState(CurrentQuestFilter, Quests);

	for (const FTQuestStruct& Quest : Quests)
	{
		UFTQuestListObject* QuestObject = NewObject<UFTQuestListObject>(this);
		QuestObject->Initialize(Quest, QuestBoard->CanCompleteQuest(Quest, PlayerInventory));
		LV_Quests->AddItem(QuestObject);

		if (Quest.QuestID == SelectedQuestID)
		{
			SelectedQuest = QuestObject;
			LV_Quests->SetItemSelection(QuestObject, true);
		}
	}
}

void UFTHubQuestPanelWidget::UpdateSelectedQuestDetails()
{
	const bool bHasSelection = SelectedQuest != nullptr;

	if (TXT_SelectedQuestName)
	{
		TXT_SelectedQuestName->SetText(bHasSelection
			? SelectedQuest->GetQuest().QuestName
			: FText::FromString(TEXT("Select Quest")));
	}

	if (TXT_QuestDescription)
	{
		TXT_QuestDescription->SetText(bHasSelection
			? SelectedQuest->GetQuest().Description
			: FText::GetEmpty());
	}

	RefreshRequiredItems();
	RefreshRewardItems();

	if (BTN_CompleteQuest)
	{
		BTN_CompleteQuest->SetIsEnabled(
			bHasSelection &&
			CurrentQuestFilter == EFTQuestStateType::Active &&
			SelectedQuest->CanComplete()
		);
	}

	if (BTN_AcceptQuest)
	{
		BTN_AcceptQuest->SetIsEnabled(
			bHasSelection &&
			CurrentQuestFilter == EFTQuestStateType::Available
		);
	}
}

void UFTHubQuestPanelWidget::RefreshRequiredItems()
{
	if (!TV_RequiredItems)
	{
		return;
	}

	TV_RequiredItems->ClearListItems();

	if (!SelectedQuest)
	{
		return;
	}

	for (const FTCraftIngredientStruct& RequiredItem : SelectedQuest->GetQuest().RequiredItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(RequiredItem);
		TV_RequiredItems->AddItem(ItemObject);
	}
}

void UFTHubQuestPanelWidget::RefreshRewardItems()
{
	if (!TV_RewardItems)
	{
		return;
	}

	TV_RewardItems->ClearListItems();

	if (!SelectedQuest)
	{
		return;
	}

	for (const FTCraftIngredientStruct& RewardItem : SelectedQuest->GetQuest().RewardItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeIngredient(RewardItem);
		TV_RewardItems->AddItem(ItemObject);
	}
}

void UFTHubQuestPanelWidget::HandleQuestClicked(UObject* Item)
{
	SelectedQuest = Cast<UFTQuestListObject>(Item);
	UpdateSelectedQuestDetails();
}

void UFTHubQuestPanelWidget::SetQuestFilter(EFTQuestStateType NewQuestFilter)
{
	CurrentQuestFilter = NewQuestFilter;
	SelectedQuest = nullptr;
	RefreshQuestList();
	UpdateSelectedQuestDetails();
}

void UFTHubQuestPanelWidget::HandleCompleteQuestClicked()
{
	if (!QuestBoard || !SelectedQuest)
	{
		return;
	}

	if (QuestBoard->TryCompleteQuest(SelectedQuest->GetQuest().QuestID, PlayerInventory))
	{
		RefreshQuestList();
		UpdateSelectedQuestDetails();
	}
}

void UFTHubQuestPanelWidget::HandleAcceptQuestClicked()
{
	if (!QuestBoard || !SelectedQuest)
	{
		return;
	}

	if (QuestBoard->AcceptQuest(SelectedQuest->GetQuest().QuestID))
	{
		SetQuestFilter(EFTQuestStateType::Active);
	}
}

void UFTHubQuestPanelWidget::HandleAvailableQuestTabClicked()
{
	SetQuestFilter(EFTQuestStateType::Available);
}

void UFTHubQuestPanelWidget::HandleActiveQuestTabClicked()
{
	SetQuestFilter(EFTQuestStateType::Active);
}

void UFTHubQuestPanelWidget::HandleCompletedQuestTabClicked()
{
	SetQuestFilter(EFTQuestStateType::Completed);
}
