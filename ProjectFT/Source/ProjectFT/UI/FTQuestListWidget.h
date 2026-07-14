#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTQuestListWidget.generated.h"

struct FFTMessagePayloadStruct;
class UCheckBox;
class UFTInventoryComponent;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class PROJECTFT_API UFTQuestListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void RefreshQuestList();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void SetQuestTitle(const FText& NewQuestTitle);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void SetEmptyQuestVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void ClearQuestEntries();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void AddQuestEntryWidget(UWidget* QuestEntryWidget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyQuest = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_QuestList = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_QuestRow_0 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_QuestRow_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_QuestRow_2 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CB_Quest_0 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CB_Quest_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CB_Quest_2 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quest_0 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quest_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quest_2 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestCount_0 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestCount_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestCount_2 = nullptr;

	/** 에디터에서 각 퀘스트 제목 아래에 추가할 멀티라인 상세 목표 TextBlock. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestDetail_0 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestDetail_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestDetail_2 = nullptr;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void BindInventoryListeners();
	void UnbindInventoryListeners();
	void HandleQuestProgressChanged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleQuestCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void UnregisterQuestListeners();

	FGameplayMessageListenerHandle QuestProgressListenerHandle;
	FGameplayMessageListenerHandle QuestCompletedListenerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> BoundPlayerInventory = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> BoundStorageInventory = nullptr;
};
