#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "FTTradePostListObject.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class PROJECTFT_API UFTTradePostListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTTradePostStruct& InTradePost);
	const FTTradePostStruct& GetTradePost() const;

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	FText GetDescription() const { return TradePost.Description; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	FText GetItemName() const { return ItemName; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	int32 GetCount() const { return TradePost.Count; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	int32 GetPrice() const { return TradePost.Price; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	bool IsBuyRequest() const { return TradePost.bBuyRequest; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	EFTItemCategoryType GetItemCategoryType() const { return ItemCategoryType; }

	UFUNCTION(BlueprintPure, Category = "FT|Trade Post|Data")
	TSoftObjectPtr<UTexture2D> GetItemIcon() const { return ItemIcon; }

private:
	FTTradePostStruct TradePost;
	FText ItemName;
	EFTItemCategoryType ItemCategoryType = EFTItemCategoryType::None;
	TSoftObjectPtr<UTexture2D> ItemIcon;
};
