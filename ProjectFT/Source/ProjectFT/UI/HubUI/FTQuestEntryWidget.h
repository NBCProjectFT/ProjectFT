#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTQuestEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTFT_API UFTQuestEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	void ApplySelectionVisual(bool bIsSelected);
	void ApplyTextVisual(UTextBlock* TextBlock, bool bIsSelected) const;

	bool bQuestAccepted = false;
	bool bQuestCanComplete = false;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_QuestName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestSender;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestSummary;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_QuestReward;
};
