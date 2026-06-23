#include "FTStorageItemEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTStorageItemListObject.h"

void UFTStorageItemEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTStorageItemListObject* StorageItemObject = Cast<UFTStorageItemListObject>(ListItemObject);
	if (!StorageItemObject)
	{
		return;
	}

	const FTStorageItemStruct& StorageItem = StorageItemObject->GetStorageItem();
	ItemNameText->SetText(FText::FromName(StorageItem.ItemID));
	ItemCountText->SetText(FText::AsNumber(StorageItem.Count));
}
