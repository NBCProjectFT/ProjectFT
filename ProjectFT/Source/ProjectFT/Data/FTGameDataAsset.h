#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTGameDataAsset.generated.h"

class UFTItemDataAsset;
class UDataTable;
class UWorld;

class UFTEscapedRaidWidget;
class UFTFailWidget;
class UFTHubCraftTestWidget;
class UFTHubMainWidget;
class UFTHubStorageWidget;
class UFTCountdownEscapeWidget;
class UFTInventoryWidget;
class UFTLoadingWidget;
class UFTMainHUDWidget;
class UFTMainMenuWidget;
class UFTQuestListWidget;
class UMaterialInterface;
class UUserWidget;
class UFTShopDataAsset;

UCLASS(BlueprintType)
class PROJECTFT_API UFTGameDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item")
	TArray<TSoftObjectPtr<UFTItemDataAsset>> ItemDataAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Hub")
	TSoftObjectPtr<UFTShopDataAsset> HubShopDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Hub")
	TSoftObjectPtr<UDataTable> CraftRecipeDataTable;

	/** 신규/구버전 세이브의 해금 목록이 비어 있을 때 모든 레시피를 기본 공개한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Hub")
	bool bUnlockAllRecipesWhenSaveListEmpty = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	TSoftObjectPtr<UWorld> LoadingLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
	TSoftObjectPtr<UDataTable> FlowLevelRouteDataTable;

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
	TSoftClassPtr<UFTHubMainWidget> HubMainWidgetClass;

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
