#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "FTHubQuestPanelWidget.generated.h"

class UFTObjectiveSubsystem;
class UButton;
class UFTInventoryComponent;
class UFTQuestViewModel;
class UListView;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubQuestPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Quest")
	void InitializeQuestPanel(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_Quests;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_ActiveQuestTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_CompletedQuestTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ActiveQuestCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_CompletedQuestCount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestSender;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_QuestDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestObjectiveLines;

	UPROPERTY(meta = (BindWidget))
	UTileView* TV_RequiredItems;

	UPROPERTY(meta = (BindWidget))
	UTileView* TV_RewardItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestCurrencyReward;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_QuestAction;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestAction;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	void RefreshTabButtonStyles();
	void PopulateListItems(UListView* ListView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
	void PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items);
	void HandleQuestClicked(UObject* Item);

	UFUNCTION()
	void HandleQuestActionClicked();

	UFUNCTION()
	void HandleActiveQuestTabClicked();

	UFUNCTION()
	void HandleCompletedQuestTabClicked();

	UPROPERTY(Transient)
	TObjectPtr<UFTQuestViewModel> ViewModel;

	bool bRefreshingFromViewModel = false;
};
