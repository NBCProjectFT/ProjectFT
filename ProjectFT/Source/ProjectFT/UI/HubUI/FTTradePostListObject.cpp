#include "FTTradePostListObject.h"

#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"

void UFTTradePostListObject::Initialize(const FTTradePostStruct& InTradePost)
{
	TradePost = InTradePost;
	const FName ItemID = TradePost.GetResolvedItemID();
	ItemName = FText::FromName(ItemID);
	ItemCategoryType = EFTItemCategoryType::None;
	ItemIcon.Reset();

	if (const UFTItemDataAsset* ItemData = UFTItemFunctionLibrary::FindItemData(this, ItemID))
	{
		if (!ItemData->ItemData.ItemName.IsEmpty())
		{
			ItemName = ItemData->ItemData.ItemName;
		}
		ItemCategoryType = ItemData->ItemData.CategoryType;
		ItemIcon = ItemData->ItemData.ItemIcon;
	}
}

const FTTradePostStruct& UFTTradePostListObject::GetTradePost() const
{
	return TradePost;
}
