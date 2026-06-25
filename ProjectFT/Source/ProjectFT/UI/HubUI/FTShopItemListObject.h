#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "FTShopItemListObject.generated.h"

UCLASS()
class PROJECTFT_API UFTShopItemListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTShopItemStruct& InShopItem, bool bInUnlocked);
	const FTShopItemStruct& GetShopItem() const;
	bool IsUnlocked() const;

private:
	FTShopItemStruct ShopItem;
	bool bUnlocked = false;
};
