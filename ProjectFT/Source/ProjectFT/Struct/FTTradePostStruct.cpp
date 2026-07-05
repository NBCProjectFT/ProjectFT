#include "FTTradePostStruct.h"

#include "ProjectFT/Data/FTItemDataAsset.h"

FName FTTradePostStruct::GetResolvedItemID() const
{
	if (!ItemDataAsset.IsNull())
	{
		if (const UFTItemDataAsset* LoadedItemData = ItemDataAsset.LoadSynchronous())
		{
			if (!LoadedItemData->ItemData.ItemId.IsNone())
			{
				return LoadedItemData->ItemData.ItemId;
			}
		}
	}

	return ItemID;
}
