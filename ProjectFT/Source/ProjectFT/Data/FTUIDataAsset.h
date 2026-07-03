#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FTUIDataAsset.generated.h"

class UFTEscapedRaidWidget;
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

/**
 * UI 전용 Primary Data Asset.
 * GameDataAsset은 레벨/프리로드/게임 흐름을 맡고, 이 에셋은 위젯 클래스와 UI 머티리얼 참조만 맡는다.
 */
UCLASS(BlueprintType)
class PROJECTFT_API UFTUIDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Damage")
	TSoftClassPtr<UUserWidget> DamageTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Damage")
	TSoftObjectPtr<UMaterialInterface> DamageTextBackgroundMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|UI|Inventory")
	TSoftObjectPtr<UMaterialInterface> PaperFlutterMaterial;
};
