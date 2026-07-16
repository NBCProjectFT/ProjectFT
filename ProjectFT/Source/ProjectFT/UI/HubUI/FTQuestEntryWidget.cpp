#include "FTQuestEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTQuestListObject.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"

namespace
{
	FText ResolveItemName(const UObject* WorldContextObject, const FName ItemID)
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

			Summary += FString::Printf(TEXT("%s x%d"), *ResolveItemName(WorldContextObject, Item.ItemID).ToString(), Item.Count);
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

	FString BuildQuestRewardSummary(const UObject* WorldContextObject, const FTQuestStruct& Quest)
	{
		FString Summary;
		if (Quest.CurrencyReward > 0)
		{
			Summary = FString::Printf(TEXT("%d Coin"), Quest.CurrencyReward);
		}

		const FString ItemRewardSummary = BuildIngredientSummary(WorldContextObject, Quest.RewardItems);
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
	bQuestAccepted = QuestObject->IsAccepted();
	bQuestCanComplete = QuestObject->CanComplete();

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
		const FString RewardSummary = BuildQuestRewardSummary(this, Quest);
		TXT_QuestReward->SetText(RewardSummary.IsEmpty()
			? FText::GetEmpty()
			: FText::FromString(FString::Printf(TEXT("보상: %s"), *RewardSummary)));
	}

	ApplySelectionVisual(IsListItemSelected());
}

void UFTQuestEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	ApplySelectionVisual(bIsSelected);
}

void UFTQuestEntryWidget::ApplySelectionVisual(const bool bIsSelected)
{
	ApplyTextVisual(TXT_QuestName, bIsSelected);
	ApplyTextVisual(TXT_QuestSender, bIsSelected);
	ApplyTextVisual(TXT_QuestSummary, bIsSelected);
	ApplyTextVisual(TXT_QuestReward, bIsSelected);
}

void UFTQuestEntryWidget::ApplyTextVisual(UTextBlock* TextBlock, const bool bIsSelected) const
{
	if (!TextBlock)
	{
		return;
	}

	if (bQuestAccepted && bQuestCanComplete)
	{
		static const FLinearColor CompleteColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("F3B300FF")));
		TextBlock->SetColorAndOpacity(FSlateColor(CompleteColor));
		TextBlock->SetRenderOpacity(bIsSelected ? 0.72f : 1.0f);
		return;
	}

	// 읽음 여부와 무관하게 수락한 퀘스트만 어둡게 표시한다.
	const float BaseOpacity = bQuestAccepted ? 0.55f : 1.0f;
	const float TextOpacity = BaseOpacity * (bIsSelected ? 0.72f : 1.0f);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TextBlock->SetRenderOpacity(TextOpacity);
}
