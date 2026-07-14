#include "FTRaidSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTRaidSelectViewModel.h"

void UFTRaidSelectWidget::InitializeRaidSelect(UFTRaidSelectViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}

	ViewModel = InViewModel;
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}

	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BTN_Market1->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleMarket1Clicked);
	BTN_Market1->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleMarket1Clicked);
	BTN_Market2->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleMarket2Clicked);
	BTN_Market2->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleMarket2Clicked);

	if (BTN_Market3)
	{
		BTN_Market3->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleMarket3Clicked);
		BTN_Market3->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleMarket3Clicked);
	}

	BTN_ConfirmEnter->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleConfirmEnterClicked);
	BTN_ConfirmEnter->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleConfirmEnterClicked);
	BTN_CancelConfirm->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleCancelConfirmClicked);
	BTN_CancelConfirm->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleCancelConfirmClicked);
	BTN_Close->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleCloseClicked);
	BTN_Close->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleCloseClicked);

	PNL_Confirm->SetVisibility(ESlateVisibility::Collapsed);
	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}

	Super::NativeDestruct();
}

void UFTRaidSelectWidget::CloseRaidSelect()
{
	if (UFTUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->HideRaidSelect();
	}
}

void UFTRaidSelectWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	SetMarketButtonState(BTN_Market1, TXT_Market1, 0);
	SetMarketButtonState(BTN_Market2, TXT_Market2, 1);
	SetMarketButtonState(BTN_Market3, TXT_Market3, 2);

	TXT_SelectedMarket->SetText(ViewModel->GetSelectedDisplayName());
	TXT_EntryCost->SetText(ViewModel->GetSelectedEntryCostText());
	if (TXT_Status)
	{
		TXT_Status->SetText(ViewModel->GetSelectedStatusText());
	}

	const int32 SelectedIndex = ViewModel->GetSelectedOptionIndex();
	BTN_ConfirmEnter->SetIsEnabled(SelectedIndex != INDEX_NONE && ViewModel->CanEnterOption(SelectedIndex));
}

void UFTRaidSelectWidget::HandleMarket1Clicked()
{
	HandleMarketClicked(0);
}

void UFTRaidSelectWidget::HandleMarket2Clicked()
{
	HandleMarketClicked(1);
}

void UFTRaidSelectWidget::HandleMarket3Clicked()
{
	HandleMarketClicked(2);
}

void UFTRaidSelectWidget::HandleConfirmEnterClicked()
{
	if (ViewModel)
	{
		ViewModel->ConfirmSelectedOption();
	}
}

void UFTRaidSelectWidget::HandleCancelConfirmClicked()
{
	PNL_Confirm->SetVisibility(ESlateVisibility::Collapsed);
}

void UFTRaidSelectWidget::HandleCloseClicked()
{
	CloseRaidSelect();
}

void UFTRaidSelectWidget::HandleMarketClicked(const int32 OptionIndex)
{
	if (!ViewModel)
	{
		return;
	}

	FFTRaidEntranceOption Option;
	if (!ViewModel->GetOption(OptionIndex, Option))
	{
		return;
	}

	ViewModel->SelectOption(OptionIndex);
	PNL_Confirm->SetVisibility(ESlateVisibility::Visible);
}

void UFTRaidSelectWidget::SetMarketButtonState(UButton* Button, UTextBlock* Label, const int32 OptionIndex)
{
	if (!Button || !ViewModel)
	{
		return;
	}

	FFTRaidEntranceOption Option;
	const bool bHasOption = ViewModel->GetOption(OptionIndex, Option);
	Button->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Button->SetIsEnabled(bHasOption);

	if (Label && bHasOption)
	{
		Label->SetText(Option.DisplayName);
	}
}
