#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "FTItemTileListObject.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class PROJECTFT_API UFTItemTileListObject : public UObject
{
	GENERATED_BODY()

public:
	void InitializeItem(FName InItemID, int32 InCount, int32 InPrice = 0, bool bInLocked = false);
	void InitializeIngredient(const FTCraftIngredientStruct& Ingredient, int32 InOwnedCount = INDEX_NONE);
	void InitializeShopItem(const FTShopItemStruct& ShopItem, bool bInLocked);

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	FName GetItemID() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	int32 GetCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	int32 GetOwnedCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	int32 GetPrice() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	float GetUnitWeight() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	bool IsLocked() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	bool HasOwnedCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	bool IsChecked() const;

	void SetChecked(bool bInChecked);

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	bool ShouldShowSelectionCheckBox() const;

	void SetShowSelectionCheckBox(bool bInShowSelectionCheckBox);

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	const FText& GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	const FText& GetDescription() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
	FText GetCategoryText() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item Tile")
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
	bool bShowSelectionCheckBox = true;
	FText DisplayName;
	FText Description;
	TSoftObjectPtr<UTexture2D> ItemIcon;
};
