#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTMarketViewModel.generated.h"

class UFTInventoryComponent;
class UFTShopSubsystem;
class UFTTradePostListObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTMarketViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTMarketViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Market|Items")
	TArray<UObject*> GetTradePostObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Market|Selection")
	UFTTradePostListObject* GetSelectedPostObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Market|Data")
	int32 GetSelectedItemOwnedCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Market|Rules")
	bool CanTradeSelectedPost() const;

	UFUNCTION(BlueprintPure, Category = "FT|Market|State")
	bool IsBuyRequestMode() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Market")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category = "FT|Market|Mode")
	void SetBuyRequestMode(bool bInBuyRequestMode);

	UFUNCTION(BlueprintCallable, Category = "FT|Market|Selection")
	void SelectTradePostObject(UObject* ItemObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Market|Trade")
	bool TradeSelectedPost();

	UPROPERTY(BlueprintAssignable, Category = "FT|Market")
	FFTMarketViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshTradePosts();
	void RestoreSelection(FName PreviousPostID);
	void ClearSelection();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	const struct FTTradePostStruct* GetSelectedPost() const;
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopSubsystem> ShopSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TradePostObjects;

	UPROPERTY(Transient)
	TObjectPtr<UFTTradePostListObject> SelectedPostObject;

	bool bBuyRequestMode = true;
	bool bTransactionInProgress = false;
};
