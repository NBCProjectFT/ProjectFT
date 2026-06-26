#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "FTHubQuestPanelWidget.generated.h"

class AFTHubQuestBoard;
class UButton;
class UFTInventoryComponent;
class UFTQuestListObject;
class UListView;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubQuestPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Quest")
	void InitializeQuestPanel(AFTHubQuestBoard* InQuestBoard, UFTInventoryComponent* InPlayerInventory);

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
	void RefreshQuestList();
	void UpdateSelectedQuestDetails();
	void RefreshRequiredItems();
	void RefreshRewardItems();
	void HandleQuestClicked(UObject* Item);

	void SetQuestFilter(EFTQuestStateType NewQuestFilter);

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
	AFTHubQuestBoard* QuestBoard;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTQuestListObject* SelectedQuest;

	EFTQuestStateType CurrentQuestFilter = EFTQuestStateType::Available;
};
