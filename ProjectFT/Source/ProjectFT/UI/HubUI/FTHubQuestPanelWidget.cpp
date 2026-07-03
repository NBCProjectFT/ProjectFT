#include "FTHubQuestPanelWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTQuestViewModel.h"

void UFTHubQuestPanelWidget::InitializeQuestPanel(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
			{
				ViewModel = UIManager->QuestViewModel;
			}
		}
	}

	if (!ViewModel)
	{
		ViewModel = NewObject<UFTQuestViewModel>(this);
	}

	ViewModel->OnChanged.RemoveDynamic(this, &UFTHubQuestPanelWidget::RefreshFromViewModel);
	ViewModel->OnChanged.AddDynamic(this, &UFTHubQuestPanelWidget::RefreshFromViewModel);
	ViewModel->Initialize(InObjectiveSubsystem, InPlayerInventory);
	RefreshFromViewModel();
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

	RefreshFromViewModel();
}

void UFTHubQuestPanelWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	bRefreshingFromViewModel = true;
	PopulateListItems(LV_Quests, ViewModel->GetQuestObjects(), ViewModel->GetSelectedQuestObject());
	PopulateTileItems(TV_RequiredItems, ViewModel->GetRequiredItemObjects());
	PopulateTileItems(TV_RewardItems, ViewModel->GetRewardItemObjects());
	bRefreshingFromViewModel = false;

	if (TXT_SelectedQuestName)
	{
		TXT_SelectedQuestName->SetText(ViewModel->GetSelectedQuestNameText());
	}

	if (TXT_QuestDescription)
	{
		TXT_QuestDescription->SetText(ViewModel->GetSelectedQuestDescriptionText());
	}

	if (BTN_CompleteQuest)
	{
		BTN_CompleteQuest->SetIsEnabled(ViewModel->CanCompleteSelectedQuest());
	}

	if (BTN_AcceptQuest)
	{
		BTN_AcceptQuest->SetIsEnabled(ViewModel->CanAcceptSelectedQuest());
	}
}

void UFTHubQuestPanelWidget::PopulateListItems(UListView* ListView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem)
{
	if (!ListView)
	{
		return;
	}

	ListView->ClearListItems();
	for (UObject* Item : Items)
	{
		ListView->AddItem(Item);
	}

	if (SelectedItem)
	{
		ListView->SetItemSelection(SelectedItem, true);
	}

	ListView->RequestRefresh();
}

void UFTHubQuestPanelWidget::PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items)
{
	if (!TileView)
	{
		return;
	}

	TileView->ClearListItems();
	for (UObject* Item : Items)
	{
		TileView->AddItem(Item);
	}

	TileView->RequestRefresh();
}

void UFTHubQuestPanelWidget::HandleQuestClicked(UObject* Item)
{
	if (bRefreshingFromViewModel || !ViewModel)
	{
		return;
	}

	ViewModel->SelectQuestObject(Item);
}

void UFTHubQuestPanelWidget::HandleCompleteQuestClicked()
{
	if (ViewModel)
	{
		ViewModel->CompleteSelectedQuest();
	}
}

void UFTHubQuestPanelWidget::HandleAcceptQuestClicked()
{
	if (ViewModel)
	{
		ViewModel->AcceptSelectedQuest();
	}
}

void UFTHubQuestPanelWidget::HandleAvailableQuestTabClicked()
{
	if (ViewModel)
	{
		ViewModel->SetQuestFilter(EFTQuestStateType::Available);
	}
}

void UFTHubQuestPanelWidget::HandleActiveQuestTabClicked()
{
	if (ViewModel)
	{
		ViewModel->SetQuestFilter(EFTQuestStateType::Active);
	}
}

void UFTHubQuestPanelWidget::HandleCompletedQuestTabClicked()
{
	if (ViewModel)
	{
		ViewModel->SetQuestFilter(EFTQuestStateType::Completed);
	}
}
