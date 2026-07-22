#include "FTHubQuestPanelWidget.h"

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
}

void UFTHubQuestPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubQuestPanelWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTHubQuestPanelWidget::RefreshFromViewModel);
	}

	RefreshFromViewModel();
}

void UFTHubQuestPanelWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubQuestPanelWidget::RefreshFromViewModel);
	}

	Super::NativeDestruct();
}

void UFTHubQuestPanelWidget::RefreshFromViewModel()
{
	if (ViewModel)
	{
		BP_OnQuestViewModelChanged(ViewModel);
	}
}
