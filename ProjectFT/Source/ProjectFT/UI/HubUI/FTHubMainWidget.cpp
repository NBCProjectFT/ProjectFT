#include "FTHubMainWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "FTHubMarketPanelWidget.h"
#include "FTHubQuestPanelWidget.h"
#include "FTHubShopPanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Hub/FTHubTerminal.h"

void UFTHubMainWidget::InitializeHubMain(
	AFTHubTerminal* InHubTerminal,
	UFTShopSubsystem* InShopSubsystem,
	UFTInventoryComponent* InPlayerInventory
)
{
	HubTerminal = InHubTerminal;
	ShopSubsystem = InShopSubsystem;
	PlayerInventory = InPlayerInventory;

	RefreshCollectionCoinText();
	if (HasDesktopAppWindows())
	{
		CloseAllApps();
	}
	else
	{
		ShowQuestPanel();
	}
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

	RefreshCollectionCoinText();
}

void UFTHubMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshCollectionCoinText();
}

void UFTHubMainWidget::RefreshCollectionCoinText()
{
	if (!TXT_CollectionCoin)
	{
		return;
	}

	const int32 CoinAmount = ShopSubsystem
		? ShopSubsystem->GetCurrencyAmount(PlayerInventory)
		: 0;

	TXT_CollectionCoin->SetText(FText::FromString(FString::Printf(TEXT("보유 현금 %d"), CoinAmount)));
}

void UFTHubMainWidget::OpenApp(EFTHubTerminalAppType AppType)
{
	if (HasDesktopAppWindows())
	{
		SetAppVisible(AppType, true);
		BringAppToFront(AppType);
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
}

void UFTHubMainWidget::CloseApp(EFTHubTerminalAppType AppType)
{
	if (HasDesktopAppWindows())
	{
		SetAppVisible(AppType, false);
	}
}

void UFTHubMainWidget::FocusApp(EFTHubTerminalAppType AppType)
{
	BringAppToFront(AppType);
}

void UFTHubMainWidget::CloseAllApps()
{
	SetAppVisible(EFTHubTerminalAppType::Quest, false);
	SetAppVisible(EFTHubTerminalAppType::Market, false);
	SetAppVisible(EFTHubTerminalAppType::Shop, false);
	NextWindowZOrder = 10;
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

void UFTHubMainWidget::SetAppVisible(EFTHubTerminalAppType AppType, bool bVisible)
{
	if (UWidget* AppWindow = GetAppWindow(AppType))
	{
		AppWindow->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
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
