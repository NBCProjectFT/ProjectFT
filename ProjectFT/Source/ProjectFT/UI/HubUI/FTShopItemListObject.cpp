#include "FTShopItemListObject.h"

void UFTShopItemListObject::Initialize(const FTShopItemStruct& InShopItem, bool bInUnlocked)
{
	ShopItem = InShopItem;
	bUnlocked = bInUnlocked;
}

const FTShopItemStruct& UFTShopItemListObject::GetShopItem() const
{
	return ShopItem;
}

bool UFTShopItemListObject::IsUnlocked() const
{
	return bUnlocked;
}
