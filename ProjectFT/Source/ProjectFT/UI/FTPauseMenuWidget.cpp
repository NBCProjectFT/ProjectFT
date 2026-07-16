#include "FTPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectFT/Core/FTGameFlowSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UFTPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

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

	if (SLD_MasterVolume)
	{
		SLD_MasterVolume->OnValueChanged.RemoveAll(this);
		SLD_MasterVolume->OnValueChanged.AddDynamic(this, &ThisClass::HandleMasterVolumeChanged);
	}

	CloseOptionsPanel();
	HideConfirm();
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
	HideConfirm();

	if (OptionsPanel)
	{
		OptionsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SetModalLayerVisible(true);
}

void UFTPauseMenuWidget::CloseOptionsPanel()
{
	if (OptionsPanel)
	{
		OptionsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!ConfirmPanel || ConfirmPanel->GetVisibility() != ESlateVisibility::Visible)
	{
		SetModalLayerVisible(false);
	}
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

void UFTPauseMenuWidget::ShowConfirm(EFTPauseMenuConfirmType ConfirmType, const FText& Message)
{
	PendingConfirmType = ConfirmType;
	CloseOptionsPanel();

	if (TXT_ConfirmMessage)
	{
		TXT_ConfirmMessage->SetText(Message);
	}

	if (ConfirmPanel)
	{
		ConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SetModalLayerVisible(true);
}

void UFTPauseMenuWidget::HideConfirm()
{
	PendingConfirmType = EFTPauseMenuConfirmType::None;

	if (ConfirmPanel)
	{
		ConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!OptionsPanel || OptionsPanel->GetVisibility() != ESlateVisibility::Visible)
	{
		SetModalLayerVisible(false);
	}
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

bool UFTPauseMenuWidget::IsCurrentFlowStateBase() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UFTGameFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTGameFlowSubsystem>() : nullptr;
	return FlowSubsystem && FlowSubsystem->GetCurrentFlowState() == EFTFlowStateType::Base;
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
		ReturnToBaseButton->SetVisibility(IsCurrentFlowStateBase() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UFTPauseMenuWidget::BroadcastFlowRequest(const FGameplayTag& RequestTag)
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(RequestTag, Payload);
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
		NSLOCTEXT("FTPauseMenu", "ReturnToBaseConfirm", "레이드를 포기하고 거점으로\n돌아가시겠습니까?"));
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
		BroadcastFlowRequest(TAG_FT_Request_Flow_ReturnToBase);
	}
	else if (ConfirmType == EFTPauseMenuConfirmType::MainMenu)
	{
		RequestClosePauseMenu();
		BroadcastFlowRequest(TAG_FT_Request_Flow_ReturnToMainMenu);
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
	if (MasterSoundMix && MasterSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MasterSoundClass, Volume, 1.0f, 0.0f, true);
		UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
	}

	OnMasterVolumeChanged(Volume);
}
