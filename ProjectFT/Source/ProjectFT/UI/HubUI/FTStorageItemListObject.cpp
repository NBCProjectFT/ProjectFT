#include "FTStorageItemListObject.h"

void UFTStorageItemListObject::Initialize(const FTStorageItemStruct& InStorageItem)
{
	StorageItem = InStorageItem;
}

const FTStorageItemStruct& UFTStorageItemListObject::GetStorageItem() const
{
	return StorageItem;
}
