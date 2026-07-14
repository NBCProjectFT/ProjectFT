#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FTLevelPreloadDataAsset.generated.h"

class UFTItemDataAsset;
class UFTInventoryPreloadDataAsset;
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

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Inventory preload assets are now managed by UFTInventoryPreloadDataAsset."))
	TArray<TSoftObjectPtr<UFTItemDataAsset>> GeneratedInventoryItemAssets;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use bUseInventoryPreloadDataAsset and InventoryPreloadDataAsset instead."))
	bool bPreloadAllInventoryItemDataAssets = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	bool bUseInventoryPreloadDataAsset = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload", meta = (EditCondition = "bUseInventoryPreloadDataAsset"))
	TSoftObjectPtr<UFTInventoryPreloadDataAsset> InventoryPreloadDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UObject>> AdditionalPreloadAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Level Preload")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	void GetPreloadAssetPaths(TArray<FSoftObjectPath>& OutAssetPaths) const;
	void GetExcludedAssetPaths(TSet<FString>& OutExcludedPathStrings) const;
};
