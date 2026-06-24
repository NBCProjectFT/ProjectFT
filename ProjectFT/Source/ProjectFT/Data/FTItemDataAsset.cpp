#include "FTItemDataAsset.h"

FPrimaryAssetId UFTItemDataAsset::GetPrimaryAssetId() const
{
	// ItemDatabase의 Key로 설정한 ItemId 혹은 에셋명을 고유 식별 명칭(Asset Name)으로 지정합니다.
	// FName("FTItemItem")은 DefaultGame.ini에 지정한 AssetType과 일치해야 합니다.
	return FPrimaryAssetId(FName("FTItemItem"), ItemData.ItemId);
}
