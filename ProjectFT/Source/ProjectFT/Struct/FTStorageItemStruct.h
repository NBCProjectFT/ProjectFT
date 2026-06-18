#pragma once

#include "CoreMinimal.h"
#include "FTStorageItemStruct.generated.h"

USTRUCT(BlueprintType)
struct FTStorageItemStruct
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;
};
