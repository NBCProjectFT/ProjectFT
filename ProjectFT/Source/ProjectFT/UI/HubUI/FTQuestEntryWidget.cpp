#include "FTQuestEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

namespace
{
	FString BuildIngredientSummary(const TArray<FTCraftIngredientStruct>& Items)
	{
		FString Summary;
		for (const FTCraftIngredientStruct& Item : Items)
		{
			if (Item.ItemID.IsNone() || Item.Count <= 0)
			{
				continue;
			}

			if (!Summary.IsEmpty())
			{
				Summary += TEXT(", ");
			}

			Summary += FString::Printf(TEXT("%s x%d"), *Item.ItemID.ToString(), Item.Count);
		}

		return Summary;
	}

	FString BuildObjectiveSummary(const FTQuestStruct& Quest)
	{
		FString Summary;
		for (const FText& ObjectiveLine : Quest.ObjectiveLines)
		{
			if (ObjectiveLine.IsEmpty())
			{
				continue;
			}

			if (!Summary.IsEmpty())
			{
				Summary += TEXT(", ");
			}

			Summary += ObjectiveLine.ToString();
		}

		return Summary.IsEmpty() ? Quest.Description.ToString() : Summary;
	}

	FString BuildQuestRewardSummary(const FTQuestStruct& Quest)
	{
		FString Summary;
		if (Quest.CurrencyReward > 0)
		{
			Summary = FString::Printf(TEXT("%d Coin"), Quest.CurrencyReward);
		}

		const FString ItemRewardSummary = BuildIngredientSummary(Quest.RewardItems);
		if (!ItemRewardSummary.IsEmpty())
		{
			if (!Summary.IsEmpty())
			{
				Summary += TEXT(", ");
			}

			Summary += ItemRewardSummary;
		}

		return Summary;
	}
}

void UFTQuestEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTQuestListObject* QuestObject = Cast<UFTQuestListObject>(ListItemObject);
	if (!QuestObject)
	{
		return;
	}

	const FTQuestStruct& Quest = QuestObject->GetQuest();

	TXT_QuestName->SetText(Quest.QuestName);

	if (TXT_QuestSender)
	{
		TXT_QuestSender->SetText(Quest.SenderName.IsEmpty()
			? FText::FromString(TEXT("Hub Mail"))
			: Quest.SenderName);
	}

	if (TXT_QuestSummary)
	{
		TXT_QuestSummary->SetText(FText::FromString(BuildObjectiveSummary(Quest)));
	}

	if (TXT_QuestReward)
	{
		const FString RewardSummary = BuildQuestRewardSummary(Quest);
		TXT_QuestReward->SetText(RewardSummary.IsEmpty()
			? FText::GetEmpty()
			: FText::FromString(FString::Printf(TEXT("보상: %s"), *RewardSummary)));
	}

	const FSlateColor TextColor = QuestObject->CanComplete()
		? FSlateColor(FLinearColor::Black)
		: FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));

	TXT_QuestName->SetColorAndOpacity(TextColor);

	if (TXT_QuestSender)
	{
		TXT_QuestSender->SetColorAndOpacity(TextColor);
	}

	if (TXT_QuestSummary)
	{
		TXT_QuestSummary->SetColorAndOpacity(TextColor);
	}

	if (TXT_QuestReward)
	{
		TXT_QuestReward->SetColorAndOpacity(TextColor);
	}
}
