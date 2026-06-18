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
};
