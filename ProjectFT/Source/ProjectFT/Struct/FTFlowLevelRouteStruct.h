#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTFlowLevelRouteStruct.generated.h"

class UFTLevelPreloadDataAsset;
class USoundBase;
class UWorld;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTFlowLevelRouteStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Flow")
	EFTFlowStateType State = EFTFlowStateType::MainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Flow")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Flow")
	bool bUseLoadingLevel = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Flow")
	TSoftObjectPtr<UFTLevelPreloadDataAsset> LevelPreloadDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Audio")
	TSoftObjectPtr<USoundBase> BGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Audio")
	float BGMFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Audio")
	float BGMFadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Audio")
	float BGMVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Audio")
	bool bKeepCurrentBGM = false;
};
