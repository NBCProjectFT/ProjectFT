#include "FTShopItemStruct.h"

#include "ProjectFT/Data/FTItemDataAsset.h"

FName FTShopItemStruct::GetResolvedItemID() const
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
