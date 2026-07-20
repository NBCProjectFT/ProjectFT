#include "FTMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

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

	BindButton(TEXT("WBP_OptionsButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsButtonClicked));
	BindButton(TEXT("WBP_OptionsBackButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleOptionsBackButtonClicked));
	BindButton(TEXT("WBP_QuitButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitButtonClicked));
	BindButton(TEXT("WBP_QuitConfirmButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitConfirmButtonClicked));
	BindButton(TEXT("WBP_QuitCancelButton"), GET_FUNCTION_NAME_CHECKED(ThisClass, HandleQuitCancelButtonClicked));

	ShowMainPanel();
	SetKeyboardFocus();
}

FReply UFTMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
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
	UUserWidget* ButtonWidget = WidgetTree
		? Cast<UUserWidget>(WidgetTree->FindWidget(WrapperWidgetName))
		: nullptr;
	UButton* Button = ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
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

UButton* UFTMainMenuWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	return UserWidget ? Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName)) : nullptr;
}

void UFTMainMenuWidget::ResolvePanels()
{
	CachedMainMenuSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("SW_MainMenuPanels")));
	CachedMainPanel = GetWidgetFromName(TEXT("MainPanel"));
	CachedOptionsPanel = GetWidgetFromName(TEXT("OptionsPanel"));
	CachedQuitConfirmPanel = GetWidgetFromName(TEXT("QuitConfirmPanel"));
	CachedQuitConfirmBorder = GetWidgetFromName(TEXT("QuitConfirmBorder"));
	
}

void UFTMainMenuWidget::ActivatePanel(UWidget* PanelToShow)
{
	if (CachedMainMenuSwitcher && PanelToShow && CachedMainMenuSwitcher->GetChildIndex(PanelToShow) != INDEX_NONE)
	{
		CachedMainMenuSwitcher->SetActiveWidget(PanelToShow);
	}
}

void UFTMainMenuWidget::ShowMainPanel()
{
	HideQuitConfirm();
	ActivatePanel(CachedMainPanel);
}

void UFTMainMenuWidget::ShowOptionsPanel()
{
	HideQuitConfirm();
	ActivatePanel(CachedOptionsPanel);
}

void UFTMainMenuWidget::ShowQuitConfirmPanel()
{
	if (CachedQuitConfirmBorder)
	{
		CachedQuitConfirmBorder->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	ActivatePanel(CachedQuitConfirmPanel);
}

void UFTMainMenuWidget::HideQuitConfirm()
{
	if (CachedQuitConfirmBorder)
	{
		CachedQuitConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UFTMainMenuWidget::IsQuitConfirmVisible() const
{
	return CachedQuitConfirmBorder
		&& CachedQuitConfirmBorder->GetVisibility() != ESlateVisibility::Collapsed
		&& CachedQuitConfirmBorder->GetVisibility() != ESlateVisibility::Hidden;
}

void UFTMainMenuWidget::HandleStartButtonClicked()
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_StartGame, Payload);
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

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_UI_MainMenu_QuitGame, Payload);

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
	}
}

void UFTMainMenuWidget::HandleQuitCancelButtonClicked()
{
	HideQuitConfirm();
}
