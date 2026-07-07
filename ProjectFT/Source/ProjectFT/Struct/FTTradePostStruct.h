#pragma once

#include "CoreMinimal.h"
#include "FTTradePostStruct.generated.h"

class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct FTTradePostStruct
{
	GENERATED_BODY()

public:
	FName GetResolvedItemID() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FName PostID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UFTItemDataAsset> ItemDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "ItemDataAsset == nullptr", EditConditionHides))
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	bool bBuyRequest = true;
};
