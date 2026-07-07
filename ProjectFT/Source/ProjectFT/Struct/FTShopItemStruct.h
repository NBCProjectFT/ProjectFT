#pragma once

#include "CoreMinimal.h"
#include "FTShopItemStruct.generated.h"

class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct FTShopItemStruct
{
	GENERATED_BODY()

public:
	FName GetResolvedItemID() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UFTItemDataAsset> ItemDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (EditCondition = "ItemDataAsset == nullptr", EditConditionHides))
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	bool bUnlockedByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	bool bFixedSlot = false;
};
