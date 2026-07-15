#include "FTRaidSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
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

	PopulateLevelList();
	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	LV_RaidLevels->OnItemClicked().RemoveAll(this);
	LV_RaidLevels->OnItemClicked().AddUObject(this, &UFTRaidSelectWidget::HandleLevelClicked);

	BTN_Enter->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleEnterClicked);
	BTN_Enter->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleEnterClicked);
	BTN_Close->OnClicked.RemoveDynamic(this, &UFTRaidSelectWidget::HandleCloseClicked);
	BTN_Close->OnClicked.AddDynamic(this, &UFTRaidSelectWidget::HandleCloseClicked);

	PopulateLevelList();
	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}
	if (LV_RaidLevels)
	{
		LV_RaidLevels->OnItemClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UFTRaidSelectWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		CloseRaidSelect();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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

	TXT_SelectedLevelName->SetText(ViewModel->GetSelectedDisplayName());
	TXT_EntryCost->SetText(ViewModel->GetSelectedEntryCostText());

	if (TXT_LevelDescription)
	{
		TXT_LevelDescription->SetText(ViewModel->GetSelectedDescription());
	}
	if (TXT_Status)
	{
		TXT_Status->SetText(ViewModel->GetSelectedStatusText());
	}
	if (IMG_LevelPreview)
	{
		if (UTexture2D* PreviewImage = ViewModel->GetSelectedPreviewImage())
		{
			IMG_LevelPreview->SetBrushFromTexture(PreviewImage);
			IMG_LevelPreview->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_LevelPreview->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (UTexture2D* RequiredItemIcon = ViewModel->GetSelectedRequiredItemIcon())
	{
		IMG_RequiredItemIcon->SetBrushFromTexture(RequiredItemIcon, true);
		IMG_RequiredItemIcon->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		IMG_RequiredItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	const int32 SelectedIndex = ViewModel->GetSelectedOptionIndex();
	BTN_Enter->SetIsEnabled(SelectedIndex != INDEX_NONE && ViewModel->CanEnterOption(SelectedIndex));
}

void UFTRaidSelectWidget::HandleEnterClicked()
{
	if (ViewModel)
	{
		ViewModel->ConfirmSelectedOption();
	}
}

void UFTRaidSelectWidget::HandleCloseClicked()
{
	CloseRaidSelect();
}

void UFTRaidSelectWidget::HandleLevelClicked(UObject* LevelObject)
{
	if (ViewModel)
	{
		ViewModel->SelectLevelObject(LevelObject);
	}
}

void UFTRaidSelectWidget::PopulateLevelList()
{
	if (!LV_RaidLevels || !ViewModel)
	{
		return;
	}

	const TArray<TObjectPtr<UObject>>& LevelObjects = ViewModel->GetLevelObjects();
	LV_RaidLevels->ClearListItems();
	for (UObject* LevelObject : LevelObjects)
	{
		LV_RaidLevels->AddItem(LevelObject);
	}

	int32 SelectedIndex = ViewModel->GetSelectedOptionIndex();
	if (SelectedIndex == INDEX_NONE && !LevelObjects.IsEmpty())
	{
		ViewModel->SelectLevelObject(LevelObjects[0]);
		SelectedIndex = ViewModel->GetSelectedOptionIndex();
	}

	if (LevelObjects.IsValidIndex(SelectedIndex))
	{
		LV_RaidLevels->SetSelectedItem(LevelObjects[SelectedIndex]);
	}
}
