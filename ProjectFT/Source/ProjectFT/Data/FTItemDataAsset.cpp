#include "FTItemDataAsset.h"

FPrimaryAssetId UFTItemDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FName("FTItemItem"), ItemData.ItemId);
}
