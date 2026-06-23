#include "FTHubQuestTestWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Hub/FTHubQuestBoard.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTHubQuestTestWidget::InitializeQuestTest(AFTHubQuestBoard* InQuestBoard)
{
	QuestBoard = InQuestBoard;
	SelectedQuest = nullptr;
	RefreshQuests();
}

void UFTHubQuestTestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_Quests)
	{
		LV_Quests->OnItemClicked().AddUObject(this, &UFTHubQuestTestWidget::HandleQuestClicked);
	}

	if (BTN_CompleteQuest)
	{
		BTN_CompleteQuest->OnClicked.AddDynamic(this, &UFTHubQuestTestWidget::HandleCompleteQuestClicked);
		BTN_CompleteQuest->SetIsEnabled(false);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubQuestTestWidget::HandleCloseClicked);
	}

	UpdateSelectedQuestDetails();
}

void UFTHubQuestTestWidget::RefreshQuests()
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
		UpdateSelectedQuestDetails();
		return;
	}

	TArray<FTQuestStruct> Quests;
	QuestBoard->GetQuestList(Quests);

	for (const FTQuestStruct& Quest : Quests)
	{
		UFTQuestListObject* QuestObject = NewObject<UFTQuestListObject>(this);
		QuestObject->Initialize(Quest, QuestBoard->CanCompleteQuest(Quest));
		LV_Quests->AddItem(QuestObject);

		if (Quest.QuestID == SelectedQuestID)
		{
			SelectedQuest = QuestObject;
			LV_Quests->SetItemSelection(QuestObject, true);
		}
	}

	UpdateSelectedQuestDetails();
}

void UFTHubQuestTestWidget::UpdateSelectedQuestDetails()
{
	if (!TXT_SelectedQuestName ||
		!TXT_SelectedQuestDescription ||
		!TXT_SelectedQuestRequiredItems ||
		!TXT_SelectedQuestRewardItems ||
		!BTN_CompleteQuest)
	{
		return;
	}

	if (!SelectedQuest)
	{
		TXT_SelectedQuestName->SetText(FText::FromString(TEXT("Select Quest")));
		TXT_SelectedQuestDescription->SetText(FText::GetEmpty());
		TXT_SelectedQuestRequiredItems->SetText(FText::GetEmpty());
		TXT_SelectedQuestRewardItems->SetText(FText::GetEmpty());
		BTN_CompleteQuest->SetIsEnabled(false);
		return;
	}

	const FTQuestStruct& Quest = SelectedQuest->GetQuest();

	FString RequiredItems;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT("\n");
		}

		RequiredItems += FString::Printf(
			TEXT("요구 아이템: %s x%d"),
			*RequiredItem.ItemID.ToString(),
			RequiredItem.Count
		);
	}

	FString RewardItems;
	for (const FTCraftIngredientStruct& RewardItem : Quest.RewardItems)
	{
		if (!RewardItems.IsEmpty())
		{
			RewardItems += TEXT("\n");
		}

		RewardItems += FString::Printf(
			TEXT("보상: %s x%d"),
			*RewardItem.ItemID.ToString(),
			RewardItem.Count
		);
	}

	TXT_SelectedQuestName->SetText(Quest.QuestName);
	TXT_SelectedQuestDescription->SetText(Quest.Description);
	TXT_SelectedQuestRequiredItems->SetText(FText::FromString(RequiredItems));
	TXT_SelectedQuestRewardItems->SetText(FText::FromString(RewardItems));
	BTN_CompleteQuest->SetIsEnabled(SelectedQuest->CanComplete());
}

void UFTHubQuestTestWidget::HandleQuestClicked(UObject* Item)
{
	SelectedQuest = Cast<UFTQuestListObject>(Item);
	UpdateSelectedQuestDetails();
}

void UFTHubQuestTestWidget::HandleCompleteQuestClicked()
{
	if (!QuestBoard || !SelectedQuest)
	{
		return;
	}

	if (QuestBoard->TryCompleteQuest(SelectedQuest->GetQuest().QuestID))
	{
		RefreshQuests();
	}
}

void UFTHubQuestTestWidget::HandleCloseClicked()
{
	if (QuestBoard)
	{
		QuestBoard->CloseQuestWidget();
	}
}