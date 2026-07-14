#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FTLevelPreloadDataAsset.generated.h"

class UFTItemDataAsset;
class UWorld;

UCLASS(BlueprintType)
class PROJECTFT_API UFTLevelPreloadDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	FName LevelId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UObject>> GeneratedEnvironmentAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UFTItemDataAsset>> GeneratedInventoryItemAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	bool bPreloadAllInventoryItemDataAssets = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UObject>> AdditionalPreloadAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	void GetPreloadAssetPaths(TArray<FSoftObjectPath>& OutAssetPaths) const;
};
