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
	UButton* BTN_AvailableQuestTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_ActiveQuestTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_CompletedQuestTab;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_QuestDescription;

	UPROPERTY(meta = (BindWidget))
	UTileView* TV_RequiredItems;

	UPROPERTY(meta = (BindWidget))
	UTileView* TV_RewardItems;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_CompleteQuest;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_AcceptQuest;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	void PopulateListItems(UListView* ListView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
	void PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items);
	void HandleQuestClicked(UObject* Item);

	UFUNCTION()
	void HandleCompleteQuestClicked();

	UFUNCTION()
	void HandleAcceptQuestClicked();

	UFUNCTION()
	void HandleAvailableQuestTabClicked();

	UFUNCTION()
	void HandleActiveQuestTabClicked();

	UFUNCTION()
	void HandleCompletedQuestTabClicked();

	UPROPERTY(Transient)
	TObjectPtr<UFTQuestViewModel> ViewModel;

	bool bRefreshingFromViewModel = false;
};
