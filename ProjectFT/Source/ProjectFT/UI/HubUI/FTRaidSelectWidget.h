#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTRaidSelectWidget.generated.h"

class UButton;
class UFTRaidSelectViewModel;
class UImage;
class UListView;
class UTextBlock;

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
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> LV_RaidLevels;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Enter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BTN_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_LevelPreview;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_RequiredItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_SelectedLevelName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_LevelDescription;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_EntryCost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Status;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UFUNCTION()
	void HandleEnterClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void HandleLevelClicked(UObject* LevelObject);
	void PopulateLevelList();
};
