#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTGameDataAsset.generated.h"

class UFTItemDataAsset;
class UFTLoadingWidget;
class UFTMainMenuWidget;
class UFTMainHUDWidget;
class UFTInventoryWidget;
class UFTQuestListWidget;
class UFTHubCraftTestWidget;
class UFTHubMainWidget;
class UFTHubStorageWidget;
class UMaterialInterface;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTLoadingWidget> LoadingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UFTQuestListWidget> QuestListWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Hub")
	TSoftClassPtr<UFTHubMainWidget> HubMainWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Hub")
	TSoftClassPtr<UFTHubStorageWidget> HubStorageWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Hub")
	TSoftClassPtr<UFTHubCraftTestWidget> HubCraftWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftClassPtr<UUserWidget> DamageTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftObjectPtr<UMaterialInterface> DamageTextBackgroundMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI")
	TSoftObjectPtr<UMaterialInterface> PaperFlutterMaterial;
};
