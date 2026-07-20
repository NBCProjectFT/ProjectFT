#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "FTShopDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTShopDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFTShopDataAsset();

	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shop")
	TArray<FTShopItemStruct> FixedShopItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shop")
	TArray<FTShopItemStruct> RandomItemPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market")
	TArray<FTTradePostStruct> MarketBuyPosts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market")
	TArray<FTTradePostStruct> MarketSellPosts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation")
	bool bGenerateMarketPostsFromTemplates = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "0"))
	int32 GeneratedMarketBuyPostCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "0"))
	int32 GeneratedMarketSellPostCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "1"))
	int32 MinGeneratedPostItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "1"))
	int32 MaxGeneratedPostItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation|Buy Requests", meta = (ClampMin = "1"))
	int32 MinGeneratedBuyRequestItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation|Buy Requests", meta = (ClampMin = "1"))
	int32 MaxGeneratedBuyRequestItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation|Sell Offers", meta = (ClampMin = "1"))
	int32 MinGeneratedSellOfferItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation|Sell Offers", meta = (ClampMin = "1"))
	int32 MaxGeneratedSellOfferItemCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "0.0"))
	float MarketBuyRequestPriceMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Generation", meta = (ClampMin = "0.0"))
	float MarketSellOfferPriceMultiplier = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Template")
	TArray<FText> PostPrefixes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Template")
	TArray<FText> BuyRequestReasons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Template")
	TArray<FText> SellOfferReasons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Market|Template")
	TArray<FText> PostEndings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shop")
	int32 RandomSlotCount = 6;

	/** Items that must not appear in the shop sell list or be sold through the shop API. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shop|Sell")
	TArray<TSoftObjectPtr<UFTItemDataAsset>> SellExcludedItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shop|Currency")
	FName CurrencyItemID = TEXT("ID_Common_Coin");
};
