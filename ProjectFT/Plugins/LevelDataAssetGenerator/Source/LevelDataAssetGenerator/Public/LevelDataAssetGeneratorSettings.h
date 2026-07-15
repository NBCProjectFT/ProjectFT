#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "LevelDataAssetGeneratorSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Level Data Asset Generator"))
class LEVELDATAASSETGENERATOR_API ULevelDataAssetGeneratorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULevelDataAssetGeneratorSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("LevelDataAssetGenerator"); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	TSoftClassPtr<UPrimaryDataAsset> GeneratedDataAssetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FDirectoryPath OutputFolder;

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FString GeneratedAssetPrefix = TEXT("DA_LevelPreload_");

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FName LevelPropertyName = TEXT("Level");

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FName LevelIdPropertyName = TEXT("LevelId");

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FName GeneratedEnvironmentAssetsPropertyName = TEXT("GeneratedEnvironmentAssets");

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FName bUseInventoryPreloadDataAssetPropertyName = TEXT("bUseInventoryPreloadDataAsset");

	UPROPERTY(EditAnywhere, Config, Category = "Generation")
	FName InventoryPreloadDataAssetPropertyName = TEXT("InventoryPreloadDataAsset");

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	TSoftClassPtr<UPrimaryDataAsset> InventoryPreloadDataAssetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	FDirectoryPath InventoryPreloadOutputFolder;

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	FString InventoryPreloadAssetName = TEXT("DA_InventoryPreload_Default");

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	FName InventoryItemAssetsPropertyName = TEXT("InventoryItemAssets");

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	FName bIncludeAllPrimaryItemAssetsPropertyName = TEXT("bIncludeAllPrimaryItemAssets");

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	bool bAssignInventoryPreloadToLevelDataAssets = true;

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	FDirectoryPath InventoryItemDataDirectory;

	UPROPERTY(EditAnywhere, Config, Category = "Inventory")
	TSoftClassPtr<UPrimaryDataAsset> InventoryItemDataAssetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Filtering")
	TArray<FDirectoryPath> IgnoredPaths;

	UPROPERTY(EditAnywhere, Config, Category = "Filtering")
	bool bIgnoreEngineAssets = true;

	UPROPERTY(EditAnywhere, Config, Category = "Filtering")
	bool bIgnorePluginAssets = true;

	UPROPERTY(EditAnywhere, Config, Category = "Level Actor Scan")
	bool bIncludeStaticMeshComponent = true;

	UPROPERTY(EditAnywhere, Config, Category = "Level Actor Scan")
	bool bIncludeInstancedStaticMeshComponent = true;

	UPROPERTY(EditAnywhere, Config, Category = "Level Actor Scan")
	bool bIncludeHierarchicalInstancedStaticMeshComponent = true;
};
