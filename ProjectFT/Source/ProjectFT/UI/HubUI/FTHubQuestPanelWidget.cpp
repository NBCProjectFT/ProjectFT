#include "FTHubQuestPanelWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTQuestViewModel.h"

namespace
{
	const FLinearColor QuestTabNormalTint = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("0E1315FF")));
	const FLinearColor QuestTabSelectedTint = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("182126FF")));

	void ApplyQuestTabNormalTint(UButton* Button, const bool bSelected)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(bSelected ? QuestTabSelectedTint : QuestTabNormalTint);
		Button->SetStyle(ButtonStyle);
	}
}

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

	if (BTN_QuestAction)
	{
		BTN_QuestAction->OnClicked.RemoveDynamic(this, &UFTHubQuestPanelWidget::HandleQuestActionClicked);
		BTN_QuestAction->OnClicked.AddDynamic(this, &UFTHubQuestPanelWidget::HandleQuestActionClicked);
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

	if (TXT_ActiveQuestCount)
	{
		TXT_ActiveQuestCount->SetText(ViewModel->GetActiveQuestCountText());
	}

	if (TXT_CompletedQuestCount)
	{
		TXT_CompletedQuestCount->SetText(ViewModel->GetCompletedQuestCountText());
	}

	RefreshTabButtonStyles();

	if (TXT_QuestSender)
	{
		TXT_QuestSender->SetText(ViewModel->GetSelectedQuestSenderText());
	}

	if (TXT_QuestDescription)
	{
		TXT_QuestDescription->SetText(ViewModel->GetSelectedQuestDescriptionText());
	}

	if (TXT_QuestObjectiveLines)
	{
		TXT_QuestObjectiveLines->SetText(ViewModel->GetSelectedQuestObjectiveLinesText());
	}

	if (TV_RequiredItems)
	{
		TV_RequiredItems->SetVisibility(ViewModel->HasSelectedQuestRequiredItems()
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}

	if (TV_RewardItems)
	{
		TV_RewardItems->SetVisibility(ViewModel->GetRewardItemObjects().IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (TXT_QuestCurrencyReward)
	{
		const FText CurrencyRewardText = ViewModel->GetSelectedQuestCurrencyRewardText();
		TXT_QuestCurrencyReward->SetText(CurrencyRewardText);
		TXT_QuestCurrencyReward->SetVisibility(CurrencyRewardText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (BTN_QuestAction)
	{
		BTN_QuestAction->SetIsEnabled(ViewModel->CanExecuteSelectedQuestAction());
	}

	if (TXT_QuestAction)
	{
		TXT_QuestAction->SetText(ViewModel->GetSelectedQuestActionText());
	}
}

void UFTHubQuestPanelWidget::RefreshTabButtonStyles()
{
	if (!ViewModel)
	{
		return;
	}

	ApplyQuestTabNormalTint(BTN_ActiveQuestTab, ViewModel->IsActiveQuestTabSelected());
	ApplyQuestTabNormalTint(BTN_CompletedQuestTab, ViewModel->IsCompletedQuestTabSelected());
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

void UFTHubQuestPanelWidget::HandleQuestActionClicked()
{
	if (ViewModel)
	{
		ViewModel->ExecuteSelectedQuestAction();
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
