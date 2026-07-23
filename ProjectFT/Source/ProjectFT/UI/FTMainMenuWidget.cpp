#include "FTMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Core/FTSaveSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

DEFINE_LOG_CATEGORY_STATIC(LogFTMainMenu, Log, All);

void UFTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	ResolvePanels();

	bool bStartBound = BindButton(
		TEXT("WBP_StartButton"),
		GET_FUNCTION_NAME_CHECKED(ThisClass, HandleStartButtonClicked));
	if (!bStartBound)
	{
		bStartBound = BindButton(
			TEXT("asd"),
			GET_FUNCTION_NAME_CHECKED(ThisClass, HandleStartButtonClicked));
		if (bStartBound)
		{
			UE_LOG(LogFTMainMenu, Warning,
				TEXT("Legacy WBP_Button name 'asd' is used as Start. Rename it to WBP_StartButton."));
		}
	}
	if (!bStartBound)
	{
		UE_LOG(LogFTMainMenu, Warning,
			TEXT("Required WBP_Button 'WBP_StartButton' or its inner FTGameButton was not found."));
	}

	BindButton(TEXT("WBP_ContinueButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleContinueButtonClicked));
	CachedContinueButtonWidget = ResolveWrappedUserWidget(TEXT("WBP_ContinueButton"));
	CachedContinueButton = ResolveWrappedButton(TEXT("WBP_ContinueButton"));

	BindButton(TEXT("WBP_OptionsButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsButtonClicked));
	BindButton(TEXT("WBP_OptionsBackButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsBackButtonClicked));
	BindButton(TEXT("WBP_QuitButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitButtonClicked));
	BindButton(TEXT("WBP_QuitConfirmButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitConfirmButtonClicked));
	BindButton(TEXT("WBP_QuitCancelButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitCancelButtonClicked));
	BindButton(TEXT("WBP_NewGameConfirmButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleNewGameConfirmButtonClicked));
	BindButton(TEXT("WBP_NewGameCancelButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleNewGameCancelButtonClicked));

	if (SLD_MasterVolume)
	{
		SLD_MasterVolume->OnValueChanged.RemoveAll(this);
		SLD_MasterVolume->OnValueChanged.AddDynamic(this, &ThisClass::HandleMasterVolumeChanged);
		UpdateMasterVolumeText(SLD_MasterVolume->GetValue());
	}

	ShowMainPanel();
	RefreshSaveState();
	SetKeyboardFocus();
}

FReply UFTMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (IsNewGameConfirmVisible())
		{
			HideNewGameConfirm();
			return FReply::Handled();
		}

		if (IsQuitConfirmVisible())
		{
			HideQuitConfirm();
			return FReply::Handled();
		}

		if (CachedMainMenuSwitcher
			&& CachedMainMenuSwitcher->GetActiveWidget() != CachedMainPanel)
		{
			ShowMainPanel();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UFTMainMenuWidget::BindButton(FName WrapperWidgetName, FName HandlerName, bool bRequired)
{
	UButton* Button = ResolveWrappedButton(WrapperWidgetName);
	if (!Button)
	{
		if (bRequired)
		{
			UE_LOG(LogFTMainMenu, Warning,
				TEXT("Required WBP_Button '%s' or its inner FTGameButton was not found in WBP_MainMenu."),
				*WrapperWidgetName.ToString());
		}
		return false;
	}

	Button->OnClicked.RemoveAll(this);

	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, HandlerName);
	Button->OnClicked.Add(Delegate);
	return true;
}

UButton* UFTMainMenuWidget::ResolveWrappedButton(FName WrapperWidgetName) const
{
	return ResolveButtonInsideWidget(ResolveWrappedUserWidget(WrapperWidgetName), TEXT("FTGameButton"));
}

UUserWidget* UFTMainMenuWidget::ResolveWrappedUserWidget(FName WrapperWidgetName) const
{
	return WidgetTree
		? Cast<UUserWidget>(WidgetTree->FindWidget(WrapperWidgetName))
		: nullptr;
}

UButton* UFTMainMenuWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	return UserWidget ? Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName)) : nullptr;
}

void UFTMainMenuWidget::ResolvePanels()
{
	CachedMainMenuSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("SW_MainMenuPanels")));
	CachedMainPanel = GetWidgetFromName(TEXT("MainPanel"));
	CachedOptionsPanel = GetWidgetFromName(TEXT("OptionsPanel"));
	CachedMainActionSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("SW_MainMenuActionPanels")));
	CachedLegacyMainActionSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("WidgetSwitcher_924")));
	CachedMainActionPanel = GetWidgetFromName(TEXT("MainActionPanel"));
	if (!CachedMainActionPanel)
	{
		CachedMainActionPanel = GetWidgetFromName(TEXT("Border_268"));
	}
	if (!CachedMainActionPanel)
	{
		CachedMainActionPanel = GetWidgetFromName(TEXT("Border_352"));
	}
	CachedQuitConfirmPanel = GetWidgetFromName(TEXT("QuitConfirmPanel"));
	CachedQuitConfirmBorder = GetWidgetFromName(TEXT("QuitConfirmBorder"));
	CachedNewGameConfirmBorder = GetWidgetFromName(TEXT("NewGameConfirmBorder"));

	UE_LOG(LogFTMainMenu, Log,
		TEXT("Main menu panels resolved. MainSwitcher=%s ActionSwitcher=%s LegacyActionSwitcher=%s MainActionPanel=%s QuitConfirm=%s NewGameConfirm=%s"),
		*GetNameSafe(CachedMainMenuSwitcher),
		*GetNameSafe(CachedMainActionSwitcher),
		*GetNameSafe(CachedLegacyMainActionSwitcher),
		*GetNameSafe(CachedMainActionPanel),
		*GetNameSafe(CachedQuitConfirmBorder),
		*GetNameSafe(CachedNewGameConfirmBorder));
}

void UFTMainMenuWidget::ActivatePanel(UWidget* PanelToShow)
{
	if (CachedMainMenuSwitcher && PanelToShow && CachedMainMenuSwitcher->GetChildIndex(PanelToShow) != INDEX_NONE)
	{
		CachedMainMenuSwitcher->SetActiveWidget(PanelToShow);
	}
}

void UFTMainMenuWidget::ActivateMainActionPanel()
{
	ActivateMainActionSwitcherPanel(CachedMainActionPanel);
}

void UFTMainMenuWidget::ActivateMainActionSwitcherPanel(UWidget* PanelToShow)
{
	UWidgetSwitcher* MainActionSwitcher = GetMainActionSwitcher();
	if (MainActionSwitcher && PanelToShow && MainActionSwitcher->GetChildIndex(PanelToShow) != INDEX_NONE)
	{
		PanelToShow->SetVisibility(ESlateVisibility::Visible);
		MainActionSwitcher->SetActiveWidget(PanelToShow);
	}
}

UWidgetSwitcher* UFTMainMenuWidget::GetMainActionSwitcher() const
{
	return CachedMainActionSwitcher ? CachedMainActionSwitcher.Get() : CachedLegacyMainActionSwitcher.Get();
}

void UFTMainMenuWidget::ShowMainPanel()
{
	HideAllConfirmPanels();
	ActivatePanel(CachedMainPanel);
}

void UFTMainMenuWidget::ShowOptionsPanel()
{
	HideAllConfirmPanels();
	ActivatePanel(CachedOptionsPanel);
}

void UFTMainMenuWidget::ShowQuitConfirmPanel()
{
	HideNewGameConfirm();

	if (GetMainActionSwitcher() && CachedQuitConfirmBorder)
	{
		ActivateMainActionSwitcherPanel(CachedQuitConfirmBorder);
		return;
	}

	if (CachedQuitConfirmBorder)
	{
		CachedQuitConfirmBorder->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	ActivatePanel(CachedQuitConfirmPanel);
}

void UFTMainMenuWidget::ShowNewGameConfirmPanel()
{
	HideQuitConfirm();

	if (GetMainActionSwitcher() && CachedNewGameConfirmBorder)
	{
		ActivateMainActionSwitcherPanel(CachedNewGameConfirmBorder);
		return;
	}

	if (CachedNewGameConfirmBorder)
	{
		CachedNewGameConfirmBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void UFTMainMenuWidget::HideQuitConfirm()
{
	UWidgetSwitcher* MainActionSwitcher = GetMainActionSwitcher();
	if (MainActionSwitcher
		&& CachedQuitConfirmBorder
		&& MainActionSwitcher->GetActiveWidget() == CachedQuitConfirmBorder)
	{
		ActivateMainActionPanel();
		return;
	}

	if (CachedQuitConfirmBorder)
	{
		CachedQuitConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTMainMenuWidget::HideNewGameConfirm()
{
	UWidgetSwitcher* MainActionSwitcher = GetMainActionSwitcher();
	if (MainActionSwitcher
		&& CachedNewGameConfirmBorder
		&& MainActionSwitcher->GetActiveWidget() == CachedNewGameConfirmBorder)
	{
		ActivateMainActionPanel();
		return;
	}

	if (CachedNewGameConfirmBorder)
	{
		CachedNewGameConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTMainMenuWidget::HideAllConfirmPanels()
{
	HideQuitConfirm();
	HideNewGameConfirm();
}

bool UFTMainMenuWidget::IsQuitConfirmVisible() const
{
	if (UWidgetSwitcher* MainActionSwitcher = GetMainActionSwitcher(); MainActionSwitcher && CachedQuitConfirmBorder)
	{
		return MainActionSwitcher->GetActiveWidget() == CachedQuitConfirmBorder;
	}

	return CachedQuitConfirmBorder
		&& CachedQuitConfirmBorder->GetVisibility() != ESlateVisibility::Collapsed
		&& CachedQuitConfirmBorder->GetVisibility() != ESlateVisibility::Hidden;
}

bool UFTMainMenuWidget::IsNewGameConfirmVisible() const
{
	if (UWidgetSwitcher* MainActionSwitcher = GetMainActionSwitcher(); MainActionSwitcher && CachedNewGameConfirmBorder)
	{
		return MainActionSwitcher->GetActiveWidget() == CachedNewGameConfirmBorder;
	}

	return CachedNewGameConfirmBorder
		&& CachedNewGameConfirmBorder->GetVisibility() != ESlateVisibility::Collapsed
		&& CachedNewGameConfirmBorder->GetVisibility() != ESlateVisibility::Hidden;
}

bool UFTMainMenuWidget::HasSaveData() const
{
	UGameInstance* GameInstance = GetGameInstance();
	const UFTSaveSubsystem* SaveSubsystem = GameInstance
		? GameInstance->GetSubsystem<UFTSaveSubsystem>()
		: nullptr;
	return SaveSubsystem && SaveSubsystem->HasSaveData();
}

void UFTMainMenuWidget::RequestStartGame()
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_StartGame, Payload);
}

void UFTMainMenuWidget::HandleStartButtonClicked()
{
	RefreshSaveState();
	if (HasSaveData() && CachedNewGameConfirmBorder)
	{
		ShowNewGameConfirmPanel();
		return;
	}

	RequestStartGame();
}

void UFTMainMenuWidget::HandleContinueButtonClicked()
{
	if (!CachedContinueButton || !CachedContinueButton->GetIsEnabled())
	{
		return;
	}

	OnContinueGameRequested.Broadcast();

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_ContinueGame, Payload);
}

void UFTMainMenuWidget::SetContinueButtonEnabled(bool bEnabled)
{
	if (CachedContinueButtonWidget)
	{
		CachedContinueButtonWidget->SetIsEnabled(bEnabled);
	}

	if (CachedContinueButton)
	{
		CachedContinueButton->SetIsEnabled(bEnabled);
	}

	UE_LOG(LogFTMainMenu, Log, TEXT("Continue button enabled state refreshed. HasSaveData=%s Wrapper=%s InnerButton=%s"),
		bEnabled ? TEXT("true") : TEXT("false"),
		CachedContinueButtonWidget && CachedContinueButtonWidget->GetIsEnabled() ? TEXT("enabled") : TEXT("disabled-or-missing"),
		CachedContinueButton && CachedContinueButton->GetIsEnabled() ? TEXT("enabled") : TEXT("disabled-or-missing"));
}

void UFTMainMenuWidget::RefreshSaveState()
{
	SetContinueButtonEnabled(HasSaveData());
	HideAllConfirmPanels();
}

void UFTMainMenuWidget::HandleOptionsButtonClicked()
{
	ShowOptionsPanel();
}

void UFTMainMenuWidget::HandleOptionsBackButtonClicked()
{
	ShowMainPanel();
}

void UFTMainMenuWidget::HandleQuitButtonClicked()
{
	if (CachedQuitConfirmBorder || CachedQuitConfirmPanel)
	{
		ShowQuitConfirmPanel();
		return;
	}

	QuitGame();
}

void UFTMainMenuWidget::HandleQuitConfirmButtonClicked()
{
	QuitGame();
}

void UFTMainMenuWidget::QuitGame()
{
	OnQuitGameRequested.Broadcast();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
	}
}

void UFTMainMenuWidget::HandleQuitCancelButtonClicked()
{
	HideQuitConfirm();
}

void UFTMainMenuWidget::HandleNewGameConfirmButtonClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UFTSaveSubsystem>())
		{
			SaveSubsystem->ResetForNewGame();
		}
	}

	HideNewGameConfirm();
	SetContinueButtonEnabled(false);
	RequestStartGame();
}

void UFTMainMenuWidget::HandleNewGameCancelButtonClicked()
{
	HideNewGameConfirm();
}

void UFTMainMenuWidget::HandleMasterVolumeChanged(float Value)
{
	const float Volume = FMath::Clamp(Value, 0.0f, 1.0f);
	if (MasterSoundMix && MasterSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MasterSoundClass, Volume, 1.0f, 0.0f, true);
		UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
	}

	UpdateMasterVolumeText(Volume);
}

void UFTMainMenuWidget::UpdateMasterVolumeText(float Volume) const
{
	if (TXT_MasterVolumeValue)
	{
		const int32 Percent = FMath::RoundToInt(FMath::Clamp(Volume, 0.0f, 1.0f) * 100.0f);
		TXT_MasterVolumeValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percent)));
	}
}
