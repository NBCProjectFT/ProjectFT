#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMainWidget.generated.h"

class AFTHubTerminal;
class UButton;
class UTextBlock;
class UWidget;
class UFTHubMarketPanelWidget;
class UFTHubQuestPanelWidget;
class UFTHubShopPanelWidget;
class UFTInventoryComponent;
class UFTShopSubsystem;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class EFTHubTerminalAppType : uint8
{
	Quest,
	Market,
	Shop
};

UCLASS()
class PROJECTFT_API UFTHubMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void InitializeHubMain(
		AFTHubTerminal* InHubTerminal,
		UFTShopSubsystem* InShopSubsystem,
		UFTInventoryComponent* InPlayerInventory
	);

	UFTHubQuestPanelWidget* GetQuestPanelWidget() const;
	UFTHubMarketPanelWidget* GetMarketPanelWidget() const;
	UFTHubShopPanelWidget* GetShopPanelWidget() const;

	UFUNCTION(BlueprintCallable, Category = "Hub|Terminal")
	void OpenApp(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintCallable, Category = "Hub|Terminal")
	void CloseApp(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintCallable, Category = "Hub|Terminal")
	void FocusApp(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintCallable, Category = "Hub|Terminal")
	void CloseAllApps();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher* WidgetSwitcher_Main;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_MailTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuestTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_MarketTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_ShopTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuestAppIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_MarketAppIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_ShopAppIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_CloseQuestApp;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_CloseMarketApp;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_CloseShopApp;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Window_QuestApp;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Window_MarketApp;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Window_ShopApp;

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
	bool HasDesktopAppWindows() const;
	void SetAppVisible(EFTHubTerminalAppType AppType, bool bVisible);
	void BringAppToFront(EFTHubTerminalAppType AppType);
	UWidget* GetAppWindow(EFTHubTerminalAppType AppType) const;

	UFUNCTION()
	void HandleQuestTabClicked();

	UFUNCTION()
	void HandleMarketTabClicked();

	UFUNCTION()
	void HandleShopTabClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleQuestAppIconClicked();

	UFUNCTION()
	void HandleMarketAppIconClicked();

	UFUNCTION()
	void HandleShopAppIconClicked();

	UFUNCTION()
	void HandleCloseQuestAppClicked();

	UFUNCTION()
	void HandleCloseMarketAppClicked();

	UFUNCTION()
	void HandleCloseShopAppClicked();

	UPROPERTY(Transient)
	AFTHubTerminal* HubTerminal;

	UPROPERTY(Transient)
	UFTShopSubsystem* ShopSubsystem;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	int32 NextWindowZOrder = 10;
};
