#include "FTPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTPauseMenuViewModel.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UFTPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	GetPauseMenuViewModel();

	const bool bBoundResume = BindButton(TEXT("BTN_Resume"), TEXT("WBP_ResumeButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleResumeClicked));
	const bool bBoundMainMenu = BindButton(TEXT("BTN_MainMenu"), TEXT("WBP_MainMenuButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleMainMenuClicked));
	const bool bBoundOptions = BindButton(TEXT("BTN_Options"), TEXT("WBP_OptionsButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsClicked));
	BindButton(TEXT("BTN_OptionsClose"), TEXT("WBP_OptionsCloseButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsCloseClicked));
	const bool bBoundReturnToBase = BindButton(TEXT("BTN_ReturnToBase"), TEXT("WBP_ReturnToBaseButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleReturnToBaseClicked));
	const bool bBoundQuitGame = BindButton(TEXT("BTN_QuitGame"), TEXT("WBP_QuitGameButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitGameClicked));
	BindButton(TEXT("BTN_ConfirmYes"), TEXT("WBP_ConfirmYesButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleConfirmYesClicked));
	BindButton(TEXT("BTN_ConfirmNo"), TEXT("WBP_ConfirmNoButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleConfirmNoClicked));

	if (!bBoundResume)
	{
		BindButtonByMenuIndex(0, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleResumeClicked));
	}
	if (!bBoundMainMenu)
	{
		BindButtonByMenuIndex(1, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleMainMenuClicked));
	}
	if (!bBoundOptions)
	{
		BindButtonByMenuIndex(2, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsClicked));
	}
	if (!bBoundReturnToBase)
	{
		BindButtonByMenuIndex(3, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleReturnToBaseClicked));
	}
	if (!bBoundQuitGame)
	{
		BindButtonByMenuIndex(4, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitGameClicked));
	}

	UpdateReturnToBaseButtonVisibility();

	if (!SW_PausePanels)
	{
		SW_PausePanels = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("SW_PausePanels")));
	}
	if (!PauseMenuBox)
	{
		PauseMenuBox = GetWidgetFromName(TEXT("PauseMenuBox"));
	}
	if (!OptionsPanel)
	{
		OptionsPanel = GetWidgetFromName(TEXT("OptionsPanel"));
	}
	if (!ConfirmPanel)
	{
		ConfirmPanel = GetWidgetFromName(TEXT("ConfirmPanel"));
	}

	if (SLD_MasterVolume)
	{
		SLD_MasterVolume->OnValueChanged.RemoveAll(this);
		SLD_MasterVolume->OnValueChanged.AddDynamic(this, &ThisClass::HandleMasterVolumeChanged);
	}

	PendingConfirmType = EFTPauseMenuConfirmType::None;
	ShowPauseMenuPanel();
}

FReply UFTPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RequestClosePauseMenu();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTPauseMenuWidget::OpenOptionsPanel()
{
	PendingConfirmType = EFTPauseMenuConfirmType::None;
	ShowOptionsPanel();
	SetModalLayerVisible(true);
}

void UFTPauseMenuWidget::CloseOptionsPanel()
{
	ShowPauseMenuPanel();
}

void UFTPauseMenuWidget::RequestClosePauseMenu()
{
	CloseOptionsPanel();
	HideConfirm();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HidePauseMenu();
		}
	}
}

bool UFTPauseMenuWidget::BindButton(FName DirectButtonName, FName WrapperWidgetName, FName HandlerName)
{
	UButton* Button = ResolveButton(DirectButtonName, WrapperWidgetName);
	if (!Button)
	{
		return false;
	}

	Button->OnClicked.RemoveAll(this);

	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, HandlerName);
	Button->OnClicked.Add(Delegate);
	return true;
}

void UFTPauseMenuWidget::BindButtonByMenuIndex(int32 ButtonIndex, FName HandlerName)
{
	UButton* Button = ResolveButtonByMenuIndex(ButtonIndex);
	if (!Button)
	{
		return;
	}

	Button->OnClicked.RemoveAll(this);

	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, HandlerName);
	Button->OnClicked.Add(Delegate);
}

UButton* UFTPauseMenuWidget::ResolveButton(FName DirectButtonName, FName WrapperWidgetName) const
{
	if (WidgetTree)
	{
		if (UButton* DirectButton = Cast<UButton>(WidgetTree->FindWidget(DirectButtonName)))
		{
			return DirectButton;
		}

		if (UUserWidget* ButtonWidget = Cast<UUserWidget>(WidgetTree->FindWidget(WrapperWidgetName)))
		{
			return ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
		}
	}

	if (UButton* DirectButton = Cast<UButton>(GetWidgetFromName(DirectButtonName)))
	{
		return DirectButton;
	}

	if (UUserWidget* ButtonWidget = Cast<UUserWidget>(GetWidgetFromName(WrapperWidgetName)))
	{
		return ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
	}

	return nullptr;
}

UButton* UFTPauseMenuWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	return UserWidget ? Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName)) : nullptr;
}

UButton* UFTPauseMenuWidget::ResolveButtonByMenuIndex(int32 ButtonIndex) const
{
	const UPanelWidget* MenuBox = Cast<UPanelWidget>(GetWidgetFromName(TEXT("PauseMenuBox")));
	if (!MenuBox)
	{
		return nullptr;
	}

	int32 FoundButtonIndex = 0;
	for (int32 ChildIndex = 0; ChildIndex < MenuBox->GetChildrenCount(); ++ChildIndex)
	{
		if (UButton* Button = Cast<UButton>(MenuBox->GetChildAt(ChildIndex)))
		{
			if (FoundButtonIndex == ButtonIndex)
			{
				return Button;
			}

			++FoundButtonIndex;
		}
	}

	return nullptr;
}

void UFTPauseMenuWidget::ShowPauseMenuPanel()
{
	ActivatePanel(PauseMenuBox);
	SetModalLayerVisible(false);
}

void UFTPauseMenuWidget::ShowOptionsPanel()
{
	ActivatePanel(OptionsPanel);
}

void UFTPauseMenuWidget::ShowConfirmPanel()
{
	ActivatePanel(ConfirmPanel);
}

void UFTPauseMenuWidget::ActivatePanel(UWidget* PanelToShow)
{
	if (!PanelToShow)
	{
		return;
	}

	if (SW_PausePanels && SW_PausePanels->GetChildIndex(PanelToShow) != INDEX_NONE)
	{
		SW_PausePanels->SetActiveWidget(PanelToShow);
	}
}

void UFTPauseMenuWidget::ShowConfirm(EFTPauseMenuConfirmType ConfirmType, const FText& Message)
{
	PendingConfirmType = ConfirmType;

	if (TXT_ConfirmMessage)
	{
		TXT_ConfirmMessage->SetText(Message);
	}

	ShowConfirmPanel();
	SetModalLayerVisible(true);
}

void UFTPauseMenuWidget::HideConfirm()
{
	PendingConfirmType = EFTPauseMenuConfirmType::None;
	ShowPauseMenuPanel();
}

void UFTPauseMenuWidget::SetModalLayerVisible(bool bVisible)
{
	if (UWidget* ModalMenuBlur = GetWidgetFromName(TEXT("ModalMenuBlur")))
	{
		ModalMenuBlur->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	else if (UWidget* BlueprintModalLayer = GetWidgetFromName(TEXT("Image")))
	{
		BlueprintModalLayer->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	else if (UWidget* LegacyModalMenuBlur = GetWidgetFromName(TEXT("RuntimeModalMenuBlur")))
	{
		LegacyModalMenuBlur->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UFTPauseMenuWidget::UpdateReturnToBaseButtonVisibility()
{
	UButton* ReturnToBaseButton = ResolveButton(TEXT("BTN_ReturnToBase"), TEXT("WBP_ReturnToBaseButton"));
	if (!ReturnToBaseButton)
	{
		ReturnToBaseButton = ResolveButtonByMenuIndex(3);
	}

	if (ReturnToBaseButton)
	{
		const UFTPauseMenuViewModel* ViewModel = GetPauseMenuViewModel();
		const bool bShowReturnToBase = ViewModel && ViewModel->ShouldShowReturnToBaseButton();
		ReturnToBaseButton->SetVisibility(bShowReturnToBase ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

UFTPauseMenuViewModel* UFTPauseMenuWidget::GetPauseMenuViewModel()
{
	if (!PauseMenuViewModel)
	{
		PauseMenuViewModel = NewObject<UFTPauseMenuViewModel>(this);
	}

	if (PauseMenuViewModel)
	{
		PauseMenuViewModel->Initialize(this);
	}

	return PauseMenuViewModel;
}

void UFTPauseMenuWidget::HandleResumeClicked()
{
	RequestClosePauseMenu();
}

void UFTPauseMenuWidget::HandleMainMenuClicked()
{
	ShowConfirm(
		EFTPauseMenuConfirmType::MainMenu,
		NSLOCTEXT("FTPauseMenu", "MainMenuConfirm", "메인 메뉴로\n이동하시겠습니까?"));
}

void UFTPauseMenuWidget::HandleOptionsClicked()
{
	OpenOptionsPanel();
}

void UFTPauseMenuWidget::HandleOptionsCloseClicked()
{
	CloseOptionsPanel();
}

void UFTPauseMenuWidget::HandleReturnToBaseClicked()
{
	ShowConfirm(
		EFTPauseMenuConfirmType::ReturnToBase,
		NSLOCTEXT("FTPauseMenu", "ReturnToBaseConfirm", "인벤토리에 있는 아이템이 전부 삭제 됩니다.\n레이드를 포기하고 거점으로\n돌아가시겠습니까?"));
}

void UFTPauseMenuWidget::HandleQuitGameClicked()
{
	ShowConfirm(
		EFTPauseMenuConfirmType::QuitGame,
		NSLOCTEXT("FTPauseMenu", "QuitGameConfirm", "저장하지 않은 게임은 사라집니다.\n\n게임을 종료하시겠습니까?"));
}

void UFTPauseMenuWidget::HandleConfirmYesClicked()
{
	const EFTPauseMenuConfirmType ConfirmType = PendingConfirmType;
	HideConfirm();

	if (ConfirmType == EFTPauseMenuConfirmType::ReturnToBase)
	{
		RequestClosePauseMenu();
		if (UFTPauseMenuViewModel* ViewModel = GetPauseMenuViewModel())
		{
			ViewModel->ExecuteConfirmedAction(ConfirmType);
		}
	}
	else if (ConfirmType == EFTPauseMenuConfirmType::MainMenu)
	{
		RequestClosePauseMenu();
		if (UFTPauseMenuViewModel* ViewModel = GetPauseMenuViewModel())
		{
			ViewModel->ExecuteConfirmedAction(ConfirmType);
		}
	}
	else if (ConfirmType == EFTPauseMenuConfirmType::QuitGame)
	{
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
		}
	}
}

void UFTPauseMenuWidget::HandleConfirmNoClicked()
{
	HideConfirm();
}

void UFTPauseMenuWidget::HandleMasterVolumeChanged(float Value)
{
	const float Volume = FMath::Clamp(Value, 0.0f, 1.0f);
	if (UFTPauseMenuViewModel* ViewModel = GetPauseMenuViewModel())
	{
		ViewModel->ApplyMasterVolume(MasterSoundMix, MasterSoundClass, Volume);
	}

	OnMasterVolumeChanged(Volume);
}
