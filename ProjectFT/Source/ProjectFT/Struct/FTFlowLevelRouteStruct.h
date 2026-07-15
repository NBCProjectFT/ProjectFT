#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTFlowLevelRouteStruct.generated.h"

class UFTLevelPreloadDataAsset;
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
};
