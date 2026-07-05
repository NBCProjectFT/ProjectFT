#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "FTHubShop.generated.h"

class AFTHubStorage;
class UFTInventoryComponent;
class UFTShopSubsystem;

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
	bool CanSellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintPure, Category = "Shop")
	int32 GetShopSellPrice(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "Market")
	bool CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "Market")
	bool CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const;

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

	UFUNCTION(BlueprintPure, Category = "Shop|Currency")
	FName GetCurrencyItemID() const;

	UFUNCTION(BlueprintPure, Category = "Shop|Currency")
	int32 GetCurrencyAmount(UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Shop|Compatibility")
	AFTHubStorage* HubStorage = nullptr;

private:
	UFTShopSubsystem* GetShopSubsystem() const;
};
