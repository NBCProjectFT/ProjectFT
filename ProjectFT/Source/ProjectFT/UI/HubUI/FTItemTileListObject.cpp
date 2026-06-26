#include "FTItemTileListObject.h"

#include "Engine/AssetManager.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

void UFTItemTileListObject::InitializeItem(FName InItemID, int32 InCount, int32 InPrice, bool bInLocked)
{
	ItemID = InItemID;
	Count = FMath::Max(1, InCount);
	Price = FMath::Max(0, InPrice);
	bLocked = bInLocked;
	DisplayName = FText::FromName(ItemID);
	Description = FText::GetEmpty();
	ItemIcon.Reset();
	LoadItemData();
}

void UFTItemTileListObject::InitializeIngredient(const FTCraftIngredientStruct& Ingredient)
{
	InitializeItem(Ingredient.ItemID, Ingredient.Count);
}

void UFTItemTileListObject::InitializeShopItem(const FTShopItemStruct& ShopItem, bool bInLocked)
{
	InitializeItem(ShopItem.ItemID, ShopItem.Count, ShopItem.Price, bInLocked);
}

FName UFTItemTileListObject::GetItemID() const
{
	return ItemID;
}

int32 UFTItemTileListObject::GetCount() const
{
	return Count;
}

int32 UFTItemTileListObject::GetPrice() const
{
	return Price;
}

float UFTItemTileListObject::GetTotalWeight() const
{
	return UnitWeight * Count;
}

bool UFTItemTileListObject::IsLocked() const
{
	return bLocked;
}

bool UFTItemTileListObject::IsChecked() const
{
	return bChecked;
}

void UFTItemTileListObject::SetChecked(const bool bInChecked)
{
	bChecked = bInChecked;
}

const FText& UFTItemTileListObject::GetDisplayName() const
{
	return DisplayName;
}

const FText& UFTItemTileListObject::GetDescription() const
{
	return Description;
}

TSoftObjectPtr<UTexture2D> UFTItemTileListObject::GetItemIcon() const
{
	return ItemIcon;
}

void UFTItemTileListObject::LoadItemData()
{
	if (ItemID.IsNone())
	{
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	const FPrimaryAssetId AssetID(FName("FTItemItem"), ItemID);

	UObject* AssetObject = AssetManager.GetPrimaryAssetObject(AssetID);
	if (!AssetObject)
	{
		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetID);
		if (AssetPath.IsValid())
		{
			AssetObject = AssetPath.TryLoad();
		}
	}

	const UFTItemDataAsset* ItemDataAsset = Cast<UFTItemDataAsset>(AssetObject);
	if (!ItemDataAsset)
	{
		return;
	}

	DisplayName = ItemDataAsset->ItemData.ItemName.IsEmpty()
		? FText::FromName(ItemID)
		: ItemDataAsset->ItemData.ItemName;
	Description = ItemDataAsset->ItemData.ItemDescription;
	ItemIcon = ItemDataAsset->ItemData.ItemIcon;
	UnitWeight = FMath::Max(0.0f, ItemDataAsset->ItemData.Weight);
}
