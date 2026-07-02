#include "FTHubMainWidget.h"

#include "Components/Button.h"
#include "FTHubMarketPanelWidget.h"
#include "FTHubQuestPanelWidget.h"
#include "FTHubShopPanelWidget.h"
#include "Components/TextBlock.h"
#include "ProjectFT/Hub/FTHubShop.h"
#include "ProjectFT/Hub/FTHubTerminal.h"

void UFTHubMainWidget::InitializeHubMain(
	AFTHubTerminal* InHubTerminal,
	AFTHubQuestBoard* InQuestBoard,
	AFTHubShop* InHubShop,
	UFTInventoryComponent* InPlayerInventory
)
{
	HubTerminal = InHubTerminal;
	HubShop = InHubShop;
	PlayerInventory = InPlayerInventory;

	if (WBP_QuestPanel)
	{
		WBP_QuestPanel->InitializeQuestPanel(InQuestBoard, InPlayerInventory);
	}

	if (WBP_ShopPanel)
	{
		WBP_ShopPanel->InitializeShopPanel(InHubShop, InPlayerInventory);
	}

	if (WBP_MarketPanel)
	{
		WBP_MarketPanel->InitializeMarketPanel(InHubShop, InPlayerInventory);
	}

	RefreshCollectionCoinText();
	ShowQuestPanel();
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

	const int32 CoinAmount = HubShop
		? HubShop->GetCurrencyAmount(PlayerInventory)
		: 0;

	TXT_CollectionCoin->SetText(FText::FromString(FString::Printf(TEXT("보유 현금 %d"), CoinAmount)));
}

void UFTHubMainWidget::ShowQuestPanel()
{
	if (WBP_QuestPanel)
	{
		WBP_QuestPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (WBP_MarketPanel)
	{
		WBP_MarketPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (WBP_ShopPanel)
	{
		WBP_ShopPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHubMainWidget::ShowMarketPanel()
{
	if (WBP_QuestPanel)
	{
		WBP_QuestPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (WBP_MarketPanel)
	{
		WBP_MarketPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (WBP_ShopPanel)
	{
		WBP_ShopPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHubMainWidget::ShowShopPanel()
{
	if (WBP_QuestPanel)
	{
		WBP_QuestPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (WBP_ShopPanel)
	{
		WBP_ShopPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (WBP_MarketPanel)
	{
		WBP_MarketPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHubMainWidget::HandleQuestTabClicked()
{
	ShowQuestPanel();
}

void UFTHubMainWidget::HandleMarketTabClicked()
{
	ShowMarketPanel();
}

void UFTHubMainWidget::HandleShopTabClicked()
{
	ShowShopPanel();
}

void UFTHubMainWidget::HandleCloseClicked()
{
	if (HubTerminal)
	{
		HubTerminal->CloseHubWidget();
	}
}
