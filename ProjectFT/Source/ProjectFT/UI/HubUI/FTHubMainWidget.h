#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMainWidget.generated.h"

class AFTHubShop;
class AFTHubTerminal;
class UButton;
class UTextBlock;
class UFTHubMarketPanelWidget;
class UFTHubQuestPanelWidget;
class UFTHubShopPanelWidget;
class UFTInventoryComponent;
class UWidgetSwitcher;
UCLASS()
class PROJECTFT_API UFTHubMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void InitializeHubMain(
		AFTHubTerminal* InHubTerminal,
		AFTHubShop* InHubShop,
		UFTInventoryComponent* InPlayerInventory
	);

	UFTHubQuestPanelWidget* GetQuestPanelWidget() const;
	UFTHubMarketPanelWidget* GetMarketPanelWidget() const;
	UFTHubShopPanelWidget* GetShopPanelWidget() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* WidgetSwitcher_Main;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_MailTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuestTab;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_MarketTab;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_ShopTab;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_CollectionCoin;

	UPROPERTY(meta = (BindWidget))
	UFTHubQuestPanelWidget* WBP_QuestPanel;

	UPROPERTY(meta = (BindWidget))
	UFTHubMarketPanelWidget* WBP_MarketPanel;

	UPROPERTY(meta = (BindWidget))
	UFTHubShopPanelWidget* WBP_ShopPanel;

	
private:
	void RefreshCollectionCoinText();
	void ShowQuestPanel();
	void ShowMarketPanel();
	void ShowShopPanel();

	UFUNCTION()
	void HandleQuestTabClicked();

	UFUNCTION()
	void HandleMarketTabClicked();

	UFUNCTION()
	void HandleShopTabClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	AFTHubTerminal* HubTerminal;

	UPROPERTY(Transient)
	AFTHubShop* HubShop;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;
};
