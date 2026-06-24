#include "FTItemDataAsset.h"

FPrimaryAssetId UFTItemDataAsset::GetPrimaryAssetId() const
{
	// 에셋 매니저(Asset Manager) 시스템에서 이 데이터 에셋을 식별하고 동적으로 로드하기 위한 고유한 등록증(식별표)을 발급해주는 함수
	// FName("FTItemItem")은 DefaultGame.ini에 지정한 AssetType과 일치해야 합니다.
	return FPrimaryAssetId(FName("FTItemItem"), ItemData.ItemId);
}
