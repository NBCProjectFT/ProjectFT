#include "FTGameDataAsset.h"

const FPrimaryAssetType UFTGameDataAsset::AssetType = TEXT("FTGameData");

FPrimaryAssetId UFTGameDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
