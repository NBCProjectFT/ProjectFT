#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTQuestListWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class PROJECTFT_API UFTQuestListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void SetQuestTitle(const FText& NewQuestTitle);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void SetEmptyQuestVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void ClearQuestEntries();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void AddQuestEntryWidget(UWidget* QuestEntryWidget);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyQuest = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_QuestList = nullptr;
};
