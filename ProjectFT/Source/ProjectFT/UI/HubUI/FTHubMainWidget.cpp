#include "FTHubMainWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "FTHubMarketPanelWidget.h"
#include "FTHubQuestPanelWidget.h"
#include "FTHubShopPanelWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "InputCoreTypes.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Hub/FTHubTerminal.h"

void UFTHubMainWidget::InitializeHubMain(
	AFTHubTerminal* InHubTerminal,
	UFTShopSubsystem* InShopSubsystem,
	UFTInventoryComponent* InPlayerInventory,
	AFTHubStorage* InHubStorage
)
{
	UnbindCurrencyInventoryDelegates();

	HubTerminal = InHubTerminal;
	ShopSubsystem = InShopSubsystem;
	PlayerInventory = InPlayerInventory;
	StorageInventory = InHubStorage ? InHubStorage->GetStorageInventory() : nullptr;

	BindCurrencyInventoryDelegates();

	if (HasDesktopAppWindows())
	{
		CollapseAllAppsImmediately();
	}
	else
	{
		ShowQuestPanel();
	}

	RefreshCollectionCoinText();
}

UFTHubQuestPanelWidget* UFTHubMainWidget::GetQuestPanelWidget() const
{
	return WBP_QuestPanel;
}

UFTHubMarketPanelWidget* UFTHubMainWidget::GetMarketPanelWidget() const
{
	return WBP_MarketPanel;
}

UFTHubShopPanelWidget* UFTHubMainWidget::GetShopPanelWidget() const
{
	return WBP_ShopPanel;
}

void UFTHubMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (BTN_MailTab)
	{
		BTN_MailTab->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleQuestTabClicked);
		BTN_MailTab->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleQuestTabClicked);
	}

	if (BTN_QuestTab)
	{
		BTN_QuestTab->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleQuestTabClicked);
		BTN_QuestTab->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleQuestTabClicked);
	}

	if (BTN_MarketTab)
	{
		BTN_MarketTab->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleMarketTabClicked);
		BTN_MarketTab->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleMarketTabClicked);
	}

	if (BTN_ShopTab)
	{
		BTN_ShopTab->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleShopTabClicked);
		BTN_ShopTab->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleShopTabClicked);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleCloseClicked);
	}

	if (BTN_QuestAppIcon)
	{
		BTN_QuestAppIcon->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleQuestAppIconClicked);
		BTN_QuestAppIcon->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleQuestAppIconClicked);
	}

	if (BTN_MarketAppIcon)
	{
		BTN_MarketAppIcon->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleMarketAppIconClicked);
		BTN_MarketAppIcon->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleMarketAppIconClicked);
	}

	if (BTN_ShopAppIcon)
	{
		BTN_ShopAppIcon->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleShopAppIconClicked);
		BTN_ShopAppIcon->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleShopAppIconClicked);
	}

	if (BTN_CloseQuestApp)
	{
		BTN_CloseQuestApp->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleCloseQuestAppClicked);
		BTN_CloseQuestApp->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleCloseQuestAppClicked);
	}

	if (BTN_CloseMarketApp)
	{
		BTN_CloseMarketApp->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleCloseMarketAppClicked);
		BTN_CloseMarketApp->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleCloseMarketAppClicked);
	}

	if (BTN_CloseShopApp)
	{
		BTN_CloseShopApp->OnClicked.RemoveDynamic(this, &UFTHubMainWidget::HandleCloseShopAppClicked);
		BTN_CloseShopApp->OnClicked.AddDynamic(this, &UFTHubMainWidget::HandleCloseShopAppClicked);
	}

	if (Anim_QuestAppOpen)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleQuestAppOpenAnimationFinished);
		BindToAnimationFinished(Anim_QuestAppOpen, FinishedEvent);
	}

	if (Anim_QuestAppClose)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleQuestAppCloseAnimationFinished);
		BindToAnimationFinished(Anim_QuestAppClose, FinishedEvent);
	}

	if (Anim_MarketAppOpen)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleMarketAppOpenAnimationFinished);
		BindToAnimationFinished(Anim_MarketAppOpen, FinishedEvent);
	}

	if (Anim_MarketAppClose)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleMarketAppCloseAnimationFinished);
		BindToAnimationFinished(Anim_MarketAppClose, FinishedEvent);
	}

	if (Anim_ShopAppOpen)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleShopAppOpenAnimationFinished);
		BindToAnimationFinished(Anim_ShopAppOpen, FinishedEvent);
	}

	if (Anim_ShopAppClose)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UFTHubMainWidget::HandleShopAppCloseAnimationFinished);
		BindToAnimationFinished(Anim_ShopAppClose, FinishedEvent);
	}

	RefreshCollectionCoinText();
}

void UFTHubMainWidget::NativeDestruct()
{
	UnbindCurrencyInventoryDelegates();

	Super::NativeDestruct();
}

FReply UFTHubMainWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		HandleCloseClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTHubMainWidget::BindCurrencyInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
	}

	if (StorageInventory && StorageInventory != PlayerInventory)
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
		StorageInventory->OnInventoryChanged.AddDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
	}
}

void UFTHubMainWidget::UnbindCurrencyInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
	}

	if (StorageInventory && StorageInventory != PlayerInventory)
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubMainWidget::HandleCurrencyInventoryChanged);
	}
}

void UFTHubMainWidget::RefreshCollectionCoinText()
{
	if (!TXT_CollectionCoin)
	{
		return;
	}

	const bool bShouldShowCoinText = !HasDesktopAppWindows() || HasSettledVisibleDesktopAppWindow();
	TXT_CollectionCoin->SetVisibility(bShouldShowCoinText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShouldShowCoinText)
	{
		return;
	}

	const int32 CoinAmount = ShopSubsystem
		? ShopSubsystem->GetCurrencyAmount(PlayerInventory)
		: 0;

	TXT_CollectionCoin->SetText(FText::FromString(FString::Printf(TEXT("보유 코인 %d"), CoinAmount)));
}

void UFTHubMainWidget::OpenApp(EFTHubTerminalAppType AppType)
{
	if (HasDesktopAppWindows())
	{
		SetAppVisible(AppType, true);
		BringAppToFront(AppType);
		RefreshCollectionCoinText();
		return;
	}

	switch (AppType)
	{
	case EFTHubTerminalAppType::Quest:
		ShowQuestPanel();
		break;
	case EFTHubTerminalAppType::Market:
		ShowMarketPanel();
		break;
	case EFTHubTerminalAppType::Shop:
		ShowShopPanel();
		break;
	default:
		break;
	}

	RefreshCollectionCoinText();
}

void UFTHubMainWidget::CloseApp(EFTHubTerminalAppType AppType)
{
	if (HasDesktopAppWindows())
	{
		SetAppVisible(AppType, false);
		RefreshCollectionCoinText();
	}
}

void UFTHubMainWidget::FocusApp(EFTHubTerminalAppType AppType)
{
	BringAppToFront(AppType);
}

void UFTHubMainWidget::CloseAllApps()
{
	CloseApp(EFTHubTerminalAppType::Quest);
	CloseApp(EFTHubTerminalAppType::Market);
	CloseApp(EFTHubTerminalAppType::Shop);
	NextWindowZOrder = 10;
	RefreshCollectionCoinText();
}

void UFTHubMainWidget::ShowMarketPanel()
{
	if (WidgetSwitcher_Main && WBP_MarketPanel)
	{
		WidgetSwitcher_Main->SetActiveWidget(WBP_MarketPanel);
	}
}

void UFTHubMainWidget::ShowShopPanel()
{
	if (WidgetSwitcher_Main && WBP_ShopPanel)
	{
		WidgetSwitcher_Main->SetActiveWidget(WBP_ShopPanel);
	}
}

void UFTHubMainWidget::ShowQuestPanel()
{
	if (WidgetSwitcher_Main && WBP_QuestPanel)
	{
		WidgetSwitcher_Main->SetActiveWidget(WBP_QuestPanel);
	}
}

bool UFTHubMainWidget::HasDesktopAppWindows() const
{
	return Window_QuestApp || Window_MarketApp || Window_ShopApp;
}

bool UFTHubMainWidget::HasVisibleDesktopAppWindow() const
{
	const UWidget* AppWindows[] = { Window_QuestApp, Window_MarketApp, Window_ShopApp };
	for (const UWidget* AppWindow : AppWindows)
	{
		if (AppWindow && AppWindow->IsVisible())
		{
			return true;
		}
	}

	return false;
}

bool UFTHubMainWidget::HasSettledVisibleDesktopAppWindow() const
{
	const EFTHubTerminalAppType AppTypes[] = {
		EFTHubTerminalAppType::Quest,
		EFTHubTerminalAppType::Market,
		EFTHubTerminalAppType::Shop
	};

	for (const EFTHubTerminalAppType AppType : AppTypes)
	{
		const UWidget* AppWindow = GetAppWindow(AppType);
		if (AppWindow && AppWindow->IsVisible() && !OpeningApps.Contains(AppType) && !ClosingApps.Contains(AppType))
		{
			return true;
		}
	}

	return false;
}

void UFTHubMainWidget::SetAppVisible(EFTHubTerminalAppType AppType, bool bVisible)
{
	if (UWidget* AppWindow = GetAppWindow(AppType))
	{
		if (bVisible)
		{
			AppWindow->SetVisibility(ESlateVisibility::Visible);
			ClosingApps.Remove(AppType);
			if (UWidgetAnimation* CloseAnimation = GetAppCloseAnimation(AppType))
			{
				StopAnimation(CloseAnimation);
			}
			if (UWidgetAnimation* OpenAnimation = GetAppOpenAnimation(AppType))
			{
				OpeningApps.Add(AppType);
				RefreshCollectionCoinText();
				StopAnimation(OpenAnimation);
				PlayAnimation(OpenAnimation);
				return;
			}

			OpeningApps.Remove(AppType);
			return;
		}

		if (AppWindow->GetVisibility() == ESlateVisibility::Collapsed)
		{
			OpeningApps.Remove(AppType);
			ClosingApps.Remove(AppType);
			return;
		}

		OpeningApps.Remove(AppType);
		if (UWidgetAnimation* CloseAnimation = GetAppCloseAnimation(AppType))
		{
			ClosingApps.Add(AppType);
			RefreshCollectionCoinText();
			StopAnimation(CloseAnimation);
			PlayAnimation(CloseAnimation);
			return;
		}

		ClosingApps.Remove(AppType);
		AppWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHubMainWidget::SetAppCollapsedImmediately(EFTHubTerminalAppType AppType)
{
	if (UWidget* AppWindow = GetAppWindow(AppType))
	{
		OpeningApps.Remove(AppType);
		ClosingApps.Remove(AppType);
		AppWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHubMainWidget::CollapseAllAppsImmediately()
{
	SetAppCollapsedImmediately(EFTHubTerminalAppType::Quest);
	SetAppCollapsedImmediately(EFTHubTerminalAppType::Market);
	SetAppCollapsedImmediately(EFTHubTerminalAppType::Shop);
	NextWindowZOrder = 10;
}

void UFTHubMainWidget::BringAppToFront(EFTHubTerminalAppType AppType)
{
	UWidget* AppWindow = GetAppWindow(AppType);
	if (!AppWindow || AppWindow->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AppWindow->Slot))
	{
		CanvasSlot->SetZOrder(++NextWindowZOrder);
	}
}

UWidget* UFTHubMainWidget::GetAppWindow(EFTHubTerminalAppType AppType) const
{
	switch (AppType)
	{
	case EFTHubTerminalAppType::Quest:
		return Window_QuestApp;
	case EFTHubTerminalAppType::Market:
		return Window_MarketApp;
	case EFTHubTerminalAppType::Shop:
		return Window_ShopApp;
	default:
		return nullptr;
	}
}

UWidgetAnimation* UFTHubMainWidget::GetAppOpenAnimation(EFTHubTerminalAppType AppType) const
{
	switch (AppType)
	{
	case EFTHubTerminalAppType::Quest:
		return Anim_QuestAppOpen;
	case EFTHubTerminalAppType::Market:
		return Anim_MarketAppOpen;
	case EFTHubTerminalAppType::Shop:
		return Anim_ShopAppOpen;
	default:
		return nullptr;
	}
}

UWidgetAnimation* UFTHubMainWidget::GetAppCloseAnimation(EFTHubTerminalAppType AppType) const
{
	switch (AppType)
	{
	case EFTHubTerminalAppType::Quest:
		return Anim_QuestAppClose;
	case EFTHubTerminalAppType::Market:
		return Anim_MarketAppClose;
	case EFTHubTerminalAppType::Shop:
		return Anim_ShopAppClose;
	default:
		return nullptr;
	}
}

void UFTHubMainWidget::HandleAppCloseAnimationFinished(EFTHubTerminalAppType AppType)
{
	ClosingApps.Remove(AppType);
	SetAppCollapsedImmediately(AppType);
	RefreshCollectionCoinText();
}

void UFTHubMainWidget::HandleAppOpenAnimationFinished(EFTHubTerminalAppType AppType)
{
	OpeningApps.Remove(AppType);
	ClosingApps.Remove(AppType);
	RefreshCollectionCoinText();
}

void UFTHubMainWidget::HandleQuestTabClicked()
{
	OpenApp(EFTHubTerminalAppType::Quest);
}

void UFTHubMainWidget::HandleMarketTabClicked()
{
	OpenApp(EFTHubTerminalAppType::Market);
}

void UFTHubMainWidget::HandleShopTabClicked()
{
	OpenApp(EFTHubTerminalAppType::Shop);
}

void UFTHubMainWidget::HandleCloseClicked()
{
	if (HubTerminal)
	{
		HubTerminal->CloseHubWidget();
	}
}

void UFTHubMainWidget::HandleQuestAppIconClicked()
{
	OpenApp(EFTHubTerminalAppType::Quest);
}

void UFTHubMainWidget::HandleMarketAppIconClicked()
{
	OpenApp(EFTHubTerminalAppType::Market);
}

void UFTHubMainWidget::HandleShopAppIconClicked()
{
	OpenApp(EFTHubTerminalAppType::Shop);
}

void UFTHubMainWidget::HandleCloseQuestAppClicked()
{
	CloseApp(EFTHubTerminalAppType::Quest);
}

void UFTHubMainWidget::HandleCloseMarketAppClicked()
{
	CloseApp(EFTHubTerminalAppType::Market);
}

void UFTHubMainWidget::HandleCloseShopAppClicked()
{
	CloseApp(EFTHubTerminalAppType::Shop);
}

void UFTHubMainWidget::HandleQuestAppCloseAnimationFinished()
{
	HandleAppCloseAnimationFinished(EFTHubTerminalAppType::Quest);
}

void UFTHubMainWidget::HandleMarketAppCloseAnimationFinished()
{
	HandleAppCloseAnimationFinished(EFTHubTerminalAppType::Market);
}

void UFTHubMainWidget::HandleShopAppCloseAnimationFinished()
{
	HandleAppCloseAnimationFinished(EFTHubTerminalAppType::Shop);
}

void UFTHubMainWidget::HandleQuestAppOpenAnimationFinished()
{
	HandleAppOpenAnimationFinished(EFTHubTerminalAppType::Quest);
}

void UFTHubMainWidget::HandleMarketAppOpenAnimationFinished()
{
	HandleAppOpenAnimationFinished(EFTHubTerminalAppType::Market);
}

void UFTHubMainWidget::HandleShopAppOpenAnimationFinished()
{
	HandleAppOpenAnimationFinished(EFTHubTerminalAppType::Shop);
}

void UFTHubMainWidget::HandleCurrencyInventoryChanged()
{
	RefreshCollectionCoinText();
}
