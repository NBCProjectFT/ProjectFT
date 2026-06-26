#pragma once

#include "CoreMinimal.h"
#include "FTTradePostStruct.generated.h"

USTRUCT(BlueprintType)
struct FTTradePostStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PostID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBuyRequest = true;
};
