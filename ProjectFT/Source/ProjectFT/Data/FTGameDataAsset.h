#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "FTGameDataAsset.generated.h"

class UFTItemDataAsset;
class UFTLoadingWidget;
class UFTMainHUDWidget;
class UFTQuestListWidget;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTLoadingWidget> LoadingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTQuestListWidget> QuestListWidgetClass;
};
