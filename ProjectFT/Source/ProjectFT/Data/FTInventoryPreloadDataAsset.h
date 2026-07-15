#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FTInventoryPreloadDataAsset.generated.h"

class UFTItemDataAsset;

UCLASS(BlueprintType)
class PROJECTFT_API UFTInventoryPreloadDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory Preload", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory Preload")
	bool bIncludeAllPrimaryItemAssets = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory Preload")
	TArray<TSoftObjectPtr<UFTItemDataAsset>> InventoryItemAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory Preload")
	TArray<TSoftObjectPtr<UObject>> AdditionalPreloadAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory Preload")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	void GetPreloadAssetPaths(TArray<FSoftObjectPath>& OutAssetPaths) const;
	void GetExcludedAssetPaths(TSet<FString>& OutExcludedPathStrings) const;
};
