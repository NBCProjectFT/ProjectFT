#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "FTItemTileListObject.generated.h"

class UTexture2D;

UCLASS()
class PROJECTFT_API UFTItemTileListObject : public UObject
{
	GENERATED_BODY()

public:
	void InitializeItem(FName InItemID, int32 InCount, int32 InPrice = 0, bool bInLocked = false);
	void InitializeIngredient(const FTCraftIngredientStruct& Ingredient, int32 InOwnedCount = INDEX_NONE);
	void InitializeShopItem(const FTShopItemStruct& ShopItem, bool bInLocked);

	FName GetItemID() const;
	int32 GetCount() const;
	int32 GetOwnedCount() const;
	int32 GetPrice() const;
	float GetTotalWeight() const;
	bool IsLocked() const;
	bool HasOwnedCount() const;
	bool IsChecked() const;
	void SetChecked(bool bInChecked);
	const FText& GetDisplayName() const;
	const FText& GetDescription() const;
	FText GetCategoryText() const;
	TSoftObjectPtr<UTexture2D> GetItemIcon() const;

private:
	void LoadItemData();

	FName ItemID = NAME_None;
	int32 Count = 1;
	int32 OwnedCount = INDEX_NONE;
	int32 Price = 0;
	float UnitWeight = 0.0f;
	EFTItemCategoryType CategoryType = EFTItemCategoryType::None;
	bool bLocked = false;
	bool bChecked = false;
	FText DisplayName;
	FText Description;
	TSoftObjectPtr<UTexture2D> ItemIcon;
};
