#include "FTQuestListObject.h"

#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"

namespace
{
	FText ResolveQuestItemName(const UObject* WorldContextObject, const FName ItemID)
	{
		const UFTItemDataAsset* ItemDataAsset = UFTItemFunctionLibrary::FindItemData(WorldContextObject, ItemID);
		return ItemDataAsset && !ItemDataAsset->ItemData.ItemName.IsEmpty()
			? ItemDataAsset->ItemData.ItemName
			: FText::FromName(ItemID);
	}

	FString BuildIngredientSummary(const UObject* WorldContextObject, const TArray<FTCraftIngredientStruct>& Items)
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

			Summary += FString::Printf(TEXT("%s x%d"), *ResolveQuestItemName(WorldContextObject, Item.ItemID).ToString(), Item.Count);
		}
		return Summary;
	}
}

void UFTQuestListObject::Initialize(const FTQuestStruct& InQuest, const bool bInCanComplete, const bool bInAccepted)
{
	Quest = InQuest;
	bCanComplete = bInCanComplete;
	bAccepted = bInAccepted;

	FString ObjectiveText;
	for (const FText& ObjectiveLine : Quest.ObjectiveLines)
	{
		if (ObjectiveLine.IsEmpty())
		{
			continue;
		}

		if (!ObjectiveText.IsEmpty())
		{
			ObjectiveText += LINE_TERMINATOR;
		}
		ObjectiveText += FString::Printf(TEXT("- %s"), *ObjectiveLine.ToString());
	}
	ObjectiveLinesText = ObjectiveText.IsEmpty() ? FText::GetEmpty() : FText::FromString(ObjectiveText);
	ObjectiveSummary = ObjectiveLinesText.IsEmpty() ? Quest.Description : ObjectiveLinesText;

	FString RewardText;
	if (Quest.CurrencyReward > 0)
	{
		RewardText = FString::Printf(TEXT("%d Coin"), Quest.CurrencyReward);
	}

	const FString ItemRewardText = BuildIngredientSummary(this, Quest.RewardItems);
	if (!ItemRewardText.IsEmpty())
	{
		if (!RewardText.IsEmpty())
		{
			RewardText += TEXT(", ");
		}
		RewardText += ItemRewardText;
	}
	RewardSummary = RewardText.IsEmpty() ? FText::GetEmpty() : FText::FromString(RewardText);
}

const FTQuestStruct& UFTQuestListObject::GetQuest() const
{
	return Quest;
}

bool UFTQuestListObject::CanComplete() const
{
	return bCanComplete;
}

bool UFTQuestListObject::IsAccepted() const
{
	return bAccepted;
}
