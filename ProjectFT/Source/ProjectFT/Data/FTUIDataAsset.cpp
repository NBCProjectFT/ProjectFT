#include "FTUIDataAsset.h"

const FPrimaryAssetType UFTUIDataAsset::AssetType = TEXT("FTUIData");

FPrimaryAssetId UFTUIDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
