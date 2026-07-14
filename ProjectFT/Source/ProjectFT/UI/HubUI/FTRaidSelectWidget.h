#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTRaidSelectWidget.generated.h"

class UButton;
class UFTRaidSelectViewModel;
class UTextBlock;
class UWidget;

UCLASS(Blueprintable)
class PROJECTFT_API UFTRaidSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRaidSelect(UFTRaidSelectViewModel* InViewModel);

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	void CloseRaidSelect();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "FT|Raid")
	TObjectPtr<UFTRaidSelectViewModel> ViewModel;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Market1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Market2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Market3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_ConfirmEnter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_CancelConfirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Close;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PNL_Confirm;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Market1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Market2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Market3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_SelectedMarket;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_EntryCost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Status;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UFUNCTION()
	void HandleMarket1Clicked();

	UFUNCTION()
	void HandleMarket2Clicked();

	UFUNCTION()
	void HandleMarket3Clicked();

	UFUNCTION()
	void HandleConfirmEnterClicked();

	UFUNCTION()
	void HandleCancelConfirmClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void HandleMarketClicked(int32 OptionIndex);
	void SetMarketButtonState(UButton* Button, UTextBlock* Label, int32 OptionIndex);
};
