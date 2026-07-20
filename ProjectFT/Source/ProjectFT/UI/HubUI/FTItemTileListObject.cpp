#include "FTItemTileListObject.h"

#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"

void UFTItemTileListObject::InitializeItem(FName InItemID, int32 InCount, int32 InPrice, bool bInLocked)
{
	ItemID = InItemID;
	Count = FMath::Max(1, InCount);
	OwnedCount = INDEX_NONE;
	Price = FMath::Max(0, InPrice);
	bLocked = bInLocked;
	DisplayName = FText::FromName(ItemID);
	Description = FText::GetEmpty();
	ItemIcon.Reset();
	LoadItemData();
}

void UFTItemTileListObject::InitializeIngredient(const FTCraftIngredientStruct& Ingredient, const int32 InOwnedCount)
{
	InitializeItem(Ingredient.ItemID, Ingredient.Count);
	OwnedCount = InOwnedCount;
}

void UFTItemTileListObject::InitializeShopItem(const FTShopItemStruct& ShopItem, bool bInLocked)
{
	InitializeItem(ShopItem.GetResolvedItemID(), ShopItem.Count, ShopItem.Price, bInLocked);
}

FName UFTItemTileListObject::GetItemID() const
{
	return ItemID;
}

int32 UFTItemTileListObject::GetCount() const
{
	return Count;
}

int32 UFTItemTileListObject::GetOwnedCount() const
{
	return OwnedCount;
}

int32 UFTItemTileListObject::GetPrice() const
{
	return Price;
}

float UFTItemTileListObject::GetUnitWeight() const
{
	return UnitWeight;
}

float UFTItemTileListObject::GetTotalWeight() const
{
	return UnitWeight * Count;
}

bool UFTItemTileListObject::IsLocked() const
{
	return bLocked;
}

bool UFTItemTileListObject::HasOwnedCount() const
{
	return OwnedCount != INDEX_NONE;
}

bool UFTItemTileListObject::IsChecked() const
{
	return bChecked;
}

void UFTItemTileListObject::SetChecked(const bool bInChecked)
{
	bChecked = bInChecked;
}

bool UFTItemTileListObject::ShouldShowSelectionCheckBox() const
{
	return bShowSelectionCheckBox;
}

void UFTItemTileListObject::SetShowSelectionCheckBox(const bool bInShowSelectionCheckBox)
{
	bShowSelectionCheckBox = bInShowSelectionCheckBox;
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

FText UFTItemTileListObject::GetCategoryText() const
{
	switch (CategoryType)
	{
	case EFTItemCategoryType::Common:
		return FText::FromString(TEXT("일반"));
	case EFTItemCategoryType::Weapon:
		return FText::FromString(TEXT("무기"));
	case EFTItemCategoryType::Healing:
		return FText::FromString(TEXT("회복"));
	default:
		return FText::FromString(TEXT("기타"));
	}
}

void UFTItemTileListObject::LoadItemData()
{
	if (ItemID.IsNone())
	{
		return;
	}

	const UFTItemDataAsset* ItemDataAsset = UFTItemFunctionLibrary::FindItemData(this, ItemID);
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
	CategoryType = ItemDataAsset->ItemData.CategoryType;
}
