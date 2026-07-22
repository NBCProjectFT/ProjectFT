#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMainWidget.generated.h"

class AFTHubTerminal;
class AFTHubStorage;
class UFTObjectiveSubsystem;
class UButton;
class UTextBlock;
class UWidget;
class UFTHubMarketPanelWidget;
class UFTHubQuestPanelWidget;
class UFTHubShopPanelWidget;
class UFTInventoryComponent;
class UFTShopSubsystem;
class UWidgetAnimation;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class EFTHubTerminalAppType : uint8
{
	Quest,
	Market,
	Shop
};

UCLASS(Blueprintable)
class PROJECTFT_API UFTHubMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void InitializeHubMain(
		AFTHubTerminal* InHubTerminal,
		UFTShopSubsystem* InShopSubsystem,
		UFTInventoryComponent* InPlayerInventory,
		AFTHubStorage* InHubStorage
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

	UFUNCTION(BlueprintCallable, Category = "Hub|Terminal")
	void CloseTerminal();

	UFUNCTION(BlueprintPure, Category = "Hub|Terminal")
	int32 GetCollectionCoinAmount() const;

	UFUNCTION(BlueprintPure, Category = "Hub|Terminal|Migration")
	bool UsesBlueprintTerminalPresentation() const { return bUseBlueprintTerminalPresentation; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal", meta = (DisplayName = "On Hub Main Initialized"))
	void BP_OnHubMainInitialized(
		UFTObjectiveSubsystem* ObjectiveSubsystem,
		UFTShopSubsystem* InShopSubsystem,
		UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal", meta = (DisplayName = "On Collection Coin Changed"))
	void BP_OnCollectionCoinChanged(int32 CoinAmount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal", meta = (DisplayName = "On App Open Requested"))
	void BP_OnAppOpenRequested(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal", meta = (DisplayName = "On App Close Requested"))
	void BP_OnAppCloseRequested(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal", meta = (DisplayName = "On App Focus Requested"))
	void BP_OnAppFocusRequested(EFTHubTerminalAppType AppType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal|Audio", meta = (DisplayName = "On Hub UI Opened"))
	void BP_OnHubUIOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Terminal|Audio", meta = (DisplayName = "On Hub UI Closed"))
	void BP_OnHubUIClosed();

	void NotifyHubUIOpened();
	void NotifyHubUIClosed();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UWidgetSwitcher* WidgetSwitcher_Main;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_MailTab;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_QuestTab;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_MarketTab;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_ShopTab;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_Close;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_QuestAppIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_MarketAppIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_ShopAppIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_CloseQuestApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_CloseMarketApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UButton* BTN_CloseShopApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UWidget* Window_QuestApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UWidget* Window_MarketApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UWidget* Window_ShopApp;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Widgets", meta = (BindWidgetOptional))
	UTextBlock* TXT_CollectionCoin;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_QuestAppOpen;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_QuestAppClose;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_MarketAppOpen;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_MarketAppClose;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_ShopAppOpen;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Terminal|Animations", meta = (BindWidgetAnimOptional))
	UWidgetAnimation* Anim_ShopAppClose;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Panels", meta = (BindWidgetOptional))
	UFTHubQuestPanelWidget* WBP_QuestPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Panels", meta = (BindWidgetOptional))
	UFTHubMarketPanelWidget* WBP_MarketPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Hub|Terminal|Panels", meta = (BindWidgetOptional))
	UFTHubShopPanelWidget* WBP_ShopPanel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hub|Terminal|Migration")
	bool bUseBlueprintTerminalPresentation = false;

	
private:
	void BindCurrencyInventoryDelegates();
	void UnbindCurrencyInventoryDelegates();
	void RefreshCollectionCoinText();
	void ShowQuestPanel();
	void ShowMarketPanel();
	void ShowShopPanel();
	bool HasDesktopAppWindows() const;
	bool HasVisibleDesktopAppWindow() const;
	bool HasSettledVisibleDesktopAppWindow() const;
	void SetAppVisible(EFTHubTerminalAppType AppType, bool bVisible);
	void SetAppCollapsedImmediately(EFTHubTerminalAppType AppType);
	void CollapseAllAppsImmediately();
	void BringAppToFront(EFTHubTerminalAppType AppType);
	UWidget* GetAppWindow(EFTHubTerminalAppType AppType) const;
	UWidgetAnimation* GetAppOpenAnimation(EFTHubTerminalAppType AppType) const;
	UWidgetAnimation* GetAppCloseAnimation(EFTHubTerminalAppType AppType) const;
	void HandleAppOpenAnimationFinished(EFTHubTerminalAppType AppType);
	void HandleAppCloseAnimationFinished(EFTHubTerminalAppType AppType);

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

	UFUNCTION()
	void HandleQuestAppCloseAnimationFinished();

	UFUNCTION()
	void HandleMarketAppCloseAnimationFinished();

	UFUNCTION()
	void HandleShopAppCloseAnimationFinished();

	UFUNCTION()
	void HandleQuestAppOpenAnimationFinished();

	UFUNCTION()
	void HandleMarketAppOpenAnimationFinished();

	UFUNCTION()
	void HandleShopAppOpenAnimationFinished();

	UFUNCTION()
	void HandleCurrencyInventoryChanged();

	UPROPERTY(Transient)
	AFTHubTerminal* HubTerminal;

	UPROPERTY(Transient)
	UFTShopSubsystem* ShopSubsystem;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTInventoryComponent* StorageInventory;

	TSet<EFTHubTerminalAppType> OpeningApps;
	TSet<EFTHubTerminalAppType> ClosingApps;

	int32 NextWindowZOrder = 10;
};
