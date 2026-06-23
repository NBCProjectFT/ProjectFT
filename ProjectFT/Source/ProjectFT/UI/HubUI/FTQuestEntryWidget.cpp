#include "FTQuestEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

void UFTQuestEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTQuestListObject* QuestObject = Cast<UFTQuestListObject>(ListItemObject);
	if (!QuestObject)
	{
		return;
	}

	const FTQuestStruct& Quest = QuestObject->GetQuest();

	FString RequiredItems;
	for (const FTCraftIngredientStruct& RequiredItem : Quest.RequiredItems)
	{
		if (!RequiredItems.IsEmpty())
		{
			RequiredItems += TEXT(", ");
		}

		RequiredItems += FString::Printf(
			TEXT("요구 아이템: %s x%d"),
			*RequiredItem.ItemID.ToString(),
			RequiredItem.Count
		);
	}

	FString RewardItems;
	for (const FTCraftIngredientStruct& RewardItem : Quest.RewardItems)
	{
		if (!RewardItems.IsEmpty())
		{
			RewardItems += TEXT(", ");
		}

		RewardItems += FString::Printf(
			TEXT("보상: %s x%d"),
			*RewardItem.ItemID.ToString(),
			RewardItem.Count
		);
	}

	TXT_QuestName->SetText(Quest.QuestName);
	TXT_RequiredItems->SetText(FText::FromString(RequiredItems));
	TXT_RewardItems->SetText(FText::FromString(RewardItems));

	const FSlateColor TextColor = QuestObject->CanComplete()
		? FSlateColor(FLinearColor::Black)
		: FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));

	TXT_QuestName->SetColorAndOpacity(TextColor);
	TXT_RequiredItems->SetColorAndOpacity(TextColor);
	TXT_RewardItems->SetColorAndOpacity(TextColor);
}