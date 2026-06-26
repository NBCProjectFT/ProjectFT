#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "FTHubShop.generated.h"

class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API AFTHubShop : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubShop();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopItems();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "Shop")
	int32 GetShopSellPrice(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "Market")
	void GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	void GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void UnlockShopItem(FName ItemID);

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsShopItemUnlocked(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> FixedShopItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> RandomItemPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Market")
	TArray<FTTradePostStruct> MarketBuyPosts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Market")
	TArray<FTTradePostStruct> MarketSellPosts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 RandomSlotCount = 6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> CurrentShopItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TSet<FName> UnlockedShopItemIDs;

private:
	const FTShopItemStruct* FindCurrentShopItem(FName ItemID) const;
	const FTTradePostStruct* FindMarketBuyPost(FName PostID) const;
	const FTTradePostStruct* FindMarketSellPost(FName PostID) const;
	void PrintShopItems() const;
};
