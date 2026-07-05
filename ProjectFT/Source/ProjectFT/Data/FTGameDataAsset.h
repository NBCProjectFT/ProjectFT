#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTGameDataAsset.generated.h"

class UFTItemDataAsset;
class UFTEscapedRaidWidget;
class UFTFailWidget;
class UFTHubCraftTestWidget;
class UFTHubStorageWidget;
class UFTCountdownEscapeWidget;
class UFTInventoryWidget;
class UFTLoadingWidget;
class UFTMainHUDWidget;
class UFTMainMenuWidget;
class UFTQuestListWidget;
class UMaterialInterface;
class UUserWidget;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Core")
	TSoftClassPtr<UFTLoadingWidget> LoadingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Core")
	TSoftClassPtr<UFTMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Core")
	TSoftClassPtr<UFTMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Core")
	TSoftClassPtr<UFTInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Core")
	TSoftClassPtr<UFTQuestListWidget> QuestListWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Hub")
	TSoftClassPtr<UFTHubStorageWidget> HubStorageWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Hub")
	TSoftClassPtr<UFTHubCraftTestWidget> HubCraftWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Escape")
	TSoftClassPtr<UFTCountdownEscapeWidget> CountdownEscapeWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Escape")
	TSoftClassPtr<UFTEscapedRaidWidget> EscapedRaidWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Fail")
	TSoftClassPtr<UFTFailWidget> FailWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Damage")
	TSoftClassPtr<UUserWidget> DamageTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Damage")
	TSoftObjectPtr<UMaterialInterface> DamageTextBackgroundMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Inventory")
	TSoftObjectPtr<UMaterialInterface> PaperFlutterMaterial;
};
