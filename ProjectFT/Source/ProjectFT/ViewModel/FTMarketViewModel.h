#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTMarketViewModel.generated.h"

class AFTHubShop;
class UFTInventoryComponent;
class UFTTradePostListObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTMarketViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTMarketViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory);

	const TArray<TObjectPtr<UObject>>& GetTradePostObjects() const;
	const TArray<TObjectPtr<UObject>>& GetSelectedPostItemObjects() const;
	UFTTradePostListObject* GetSelectedPostObject() const;

	FText GetSelectedPostTitleText() const;
	FText GetSelectedPostDescriptionText() const;
	FText GetSelectedPostItemText() const;
	FText GetSelectedPostPriceText() const;
	bool CanTradeSelectedPost() const;
	bool IsBuyRequestMode() const;

	void RefreshAll();
	void SetBuyRequestMode(bool bInBuyRequestMode);
	void SelectTradePostObject(UObject* ItemObject);
	bool TradeSelectedPost();

	UPROPERTY(BlueprintAssignable, Category = "FT|Market")
	FFTMarketViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshTradePosts();
	void RefreshSelectedPostItems();
	void RestoreSelection(FName PreviousPostID);
	void ClearSelection();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	const struct FTTradePostStruct* GetSelectedPost() const;
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubShop> HubShop;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TradePostObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> SelectedPostItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<UFTTradePostListObject> SelectedPostObject;

	bool bBuyRequestMode = true;
};
