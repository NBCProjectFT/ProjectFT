#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTGameDataAsset.generated.h"

class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct FFTFlowStateDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	EFTFlowStateType State = EFTFlowStateType::MainMenu;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	FName TargetLevelName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	bool bUseLoadingLevel = true;
};

UCLASS(BlueprintType)
class PROJECTFT_API UFTGameDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Preload")
	TArray<FDirectoryPath> PreloadDirectories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Preload")
	TArray<TSoftObjectPtr<UObject>> PreloadAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Preload")
	TArray<FDirectoryPath> ExcludedDirectories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Preload")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item")
	TArray<TSoftObjectPtr<UFTItemDataAsset>> ItemDataAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	FName LoadingLevelName = TEXT("Lvl_Loading");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	TArray<FFTFlowStateDefinition> FlowStateDefinitions;
};
