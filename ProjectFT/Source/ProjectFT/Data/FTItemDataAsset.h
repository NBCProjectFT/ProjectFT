#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../Struct/FTItemDataStruct.h"
#include "FTItemDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTItemDataAsset : public UPrimaryDataAsset
{	
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FTItemDataStruct ItemData;

	/*
	 * @brief 에셋 매니저(Asset Manager) 시스템에서 이 데이터 에셋을 식별하고 동적으로 로드하기 위한 고유한 등록증(식별표)을 발급해주는 함수입니다.
	 * FName("FTItemItem")은 DefaultGame.ini에 지정한 AssetType과 일치해야 합니다.
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};