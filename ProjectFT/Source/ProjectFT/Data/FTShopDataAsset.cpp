#include "FTShopDataAsset.h"

const FPrimaryAssetType UFTShopDataAsset::AssetType = TEXT("FTShopData");

UFTShopDataAsset::UFTShopDataAsset()
{
	PostPrefixes = {
		FText::FromString(TEXT("급처")),
		FText::FromString(TEXT("거의 새상품")),
		FText::FromString(TEXT("창고정리")),
		FText::FromString(TEXT("싸게 넘김"))
	};

	BuyRequestReasons = {
		FText::FromString(TEXT("급하게 필요해서 구해봅니다")),
		FText::FromString(TEXT("허브 작업에 필요합니다")),
		FText::FromString(TEXT("남는 분 있으면 거래 부탁드립니다"))
	};

	SellOfferReasons = {
		FText::FromString(TEXT("이사 정리 중이라 내놓습니다")),
		FText::FromString(TEXT("안 써서 정리합니다")),
		FText::FromString(TEXT("급전이 필요해서 판매합니다"))
	};

	PostEndings = {
		FText::FromString(TEXT("네고 가능")),
		FText::FromString(TEXT("연락주세요")),
		FText::FromString(TEXT("빠른 거래 선호"))
	};
}

FPrimaryAssetId UFTShopDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
