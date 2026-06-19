#include "FTItemDataAsset.h"

const FPrimaryAssetType UFTItemDataAsset::AssetType = TEXT("FTItem");

FPrimaryAssetId UFTItemDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
