#pragma once

#include "CoreMinimal.h"
#include "FTShopItemStruct.generated.h"

USTRUCT(BlueprintType)
struct FTShopItemStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUnlockedByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bFixedSlot = false;
};
