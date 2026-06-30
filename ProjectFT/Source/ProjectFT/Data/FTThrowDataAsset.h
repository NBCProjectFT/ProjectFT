
#pragma once

#include "CoreMinimal.h"
#include "FTItemDataAsset.h"
#include "ProjectFT/Struct/FTThrowActorStruct.h"
#include "FTThrowDataAsset.generated.h"

UCLASS()
class PROJECTFT_API UFTThrowDataAsset : public UFTItemDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw Data")
	FFTThrowActorStruct ThrowActorData;
};
