
#pragma once

#include "CoreMinimal.h"
#include "FTItemDataAsset.h"
#include "ProjectFT/Struct/FTHitScanActionStruct.h"
#include "ProjectFT/Struct/FTCrosshairStateStruct.h"
#include "FTHitScanDataAsset.generated.h"

// 히트스캔 동작 아이템 데이터 에셋

UCLASS()
class PROJECTFT_API UFTHitScanDataAsset : public UFTItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan Data")
	FFTHitScanActionStruct HitScanActionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan Data|Crosshair")
	FTCrosshairStateStruct CrosshairData;
};
