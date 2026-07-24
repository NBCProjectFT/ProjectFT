#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Enum/FTPauseMenuConfirmType.h"
#include "FTPauseMenuWidget.generated.h"

class UButton;
class UFTPauseMenuViewModel;
class UPanelWidget;
class USlider;
class USoundClass;
class USoundMix;
class UTextBlock;
class UUserWidget;
class UWidget;
class UWidgetSwitcher;

UCLASS()
class PROJECTFT_API UFTPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void OpenOptionsPanel();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void CloseOptionsPanel();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void RequestClosePauseMenu();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<UWidgetSwitcher> SW_PausePanels;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<UWidget> PauseMenuBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<UWidget> OptionsPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<UWidget> ConfirmPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<UTextBlock> TXT_ConfirmMessage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Pause")
	TObjectPtr<USlider> SLD_MasterVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Pause|Audio")
	TObjectPtr<USoundMix> MasterSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Pause|Audio")
	TObjectPtr<USoundClass> MasterSoundClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Pause")
	void OnMasterVolumeChanged(float Volume);

protected:
	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleResumeClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleMainMenuClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleOptionsClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleOptionsCloseClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleReturnToBaseClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleQuitGameClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleConfirmYesClicked();

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void HandleConfirmNoClicked();

private:
	bool BindButton(FName DirectButtonName, FName WrapperWidgetName, FName HandlerName);
	void BindButtonByMenuIndex(int32 ButtonIndex, FName HandlerName);
	UButton* ResolveButton(FName DirectButtonName, FName WrapperWidgetName) const;
	UButton* ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const;
	UButton* ResolveButtonByMenuIndex(int32 ButtonIndex) const;
	void ShowPauseMenuPanel();
	void ShowOptionsPanel();
	void ShowConfirmPanel();
	void ActivatePanel(UWidget* PanelToShow);
	void ShowConfirm(EFTPauseMenuConfirmType ConfirmType, const FText& Message);
	void HideConfirm();
	void SetModalLayerVisible(bool bVisible);
	void UpdateReturnToBaseButtonVisibility();
	UFTPauseMenuViewModel* GetPauseMenuViewModel();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UPROPERTY(Transient)
	TObjectPtr<UFTPauseMenuViewModel> PauseMenuViewModel = nullptr;

	EFTPauseMenuConfirmType PendingConfirmType = EFTPauseMenuConfirmType::None;
};
