#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubQuestTestWidget.generated.h"

class AFTHubQuestBoard;
class UButton;
class UFTInventoryComponent;
class UFTQuestListObject;
class UListView;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTHubQuestTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Quest")
	void InitializeQuestTest(AFTHubQuestBoard* InQuestBoard, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_Quests;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestDescription;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestRequiredItems;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedQuestRewardItems;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_CompleteQuest;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	void RefreshQuests();
	void UpdateSelectedQuestDetails();
	void HandleQuestClicked(UObject* Item);

	UFUNCTION()
	void HandleCompleteQuestClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	AFTHubQuestBoard* QuestBoard;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTQuestListObject* SelectedQuest;
};
