#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMainWidget.generated.h"

class AFTHubQuestBoard;
class AFTHubShop;
class AFTHubTerminal;
class UButton;
class UFTHubMarketPanelWidget;
class UFTHubQuestPanelWidget;
class UFTHubShopPanelWidget;
class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API UFTHubMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void InitializeHubMain(
		AFTHubTerminal* InHubTerminal,
		AFTHubQuestBoard* InQuestBoard,
		AFTHubShop* InHubShop,
		UFTInventoryComponent* InPlayerInventory
	);

protected:
	virtual void NativeConstruct() override;

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

	UPROPERTY(meta = (BindWidget))
	UFTHubQuestPanelWidget* WBP_QuestPanel;

	UPROPERTY(meta = (BindWidget))
	UFTHubMarketPanelWidget* WBP_MarketPanel;

	UPROPERTY(meta = (BindWidget))
	UFTHubShopPanelWidget* WBP_ShopPanel;

private:
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
};
