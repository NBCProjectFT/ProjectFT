#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTShopSubsystem.generated.h"

struct FFTMessagePayloadStruct;
class AFTHubStorage;
class UFTInventoryComponent;
class UFTItemDataAsset;
class UFTShopDataAsset;

UCLASS()
class PROJECTFT_API UFTShopSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void ConfigureHubStorage(AFTHubStorage* InHubStorage);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void RefreshShopItems();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void RefreshShopAndMarketListings();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	bool BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	bool BuyItemCount(FName ItemID, int32 PurchaseCount, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	bool SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Shop")
	bool CanSellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop")
	int32 GetShopSellPrice(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Market")
	bool BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Market")
	bool CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Market")
	bool SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Market")
	bool CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Market")
	void GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Market")
	void GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void UnlockShopItem(FName ItemID);

	UFUNCTION(BlueprintPure, Category = "FT|Shop")
	bool IsShopItemUnlocked(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop")
	bool CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Currency")
	FName GetCurrencyItemID() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Currency")
	int32 GetCurrencyAmount(UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const;

private:
	void EnsureShopDataLoaded() const;
	void LoadShopData();
	const UFTShopDataAsset* ResolveShopDataAsset() const;
	void CollectItemDataAssets(TArray<UFTItemDataAsset*>& OutItemDataAssets) const;
	void BuildRandomItemPoolFromItemAssets();
	void GenerateMarketPostsFromTemplates();
	FTTradePostStruct BuildGeneratedMarketPost(UFTItemDataAsset& ItemDataAsset, int32 PostIndex, bool bBuyRequest) const;
	FText PickTemplateText(const TArray<FText>& Templates, const FText& FallbackText) const;
	bool IsMarketPostConsumed(FName PostID) const;
	const FTShopItemStruct* FindCurrentShopItem(FName ItemID) const;
	const FTTradePostStruct* FindMarketBuyPost(FName PostID) const;
	const FTTradePostStruct* FindMarketSellPost(FName PostID) const;
	bool IsMarketPostCountInRange(const FTTradePostStruct& Post, bool bBuyRequest) const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count) const;
	bool HasCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const;
	bool AddCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const;
	bool RemoveCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const;
	void HandleFlowStateChanged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	bool WasRaidFlowState(EFTFlowStateType FlowState) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage = nullptr;

	UPROPERTY(Transient)
	TArray<FTShopItemStruct> FixedShopItems;

	UPROPERTY(Transient)
	TArray<FTShopItemStruct> RandomItemPool;

	UPROPERTY(Transient)
	TArray<FTTradePostStruct> MarketBuyPosts;

	UPROPERTY(Transient)
	TArray<FTTradePostStruct> MarketSellPosts;

	UPROPERTY(Transient)
	TArray<FTShopItemStruct> CurrentShopItems;

	UPROPERTY(Transient)
	TSet<FName> UnlockedShopItemIDs;

	UPROPERTY(Transient)
	TSet<FName> ConsumedMarketPostIDs;

	UPROPERTY(Transient)
	TArray<FText> PostPrefixes;

	UPROPERTY(Transient)
	TArray<FText> BuyRequestReasons;

	UPROPERTY(Transient)
	TArray<FText> SellOfferReasons;

	UPROPERTY(Transient)
	TArray<FText> PostEndings;

	int32 RandomSlotCount = 6;
	FName CurrencyItemID = TEXT("ID_Common_Coin");
	bool bGenerateMarketPostsFromTemplates = true;
	int32 GeneratedMarketBuyPostCount = 6;
	int32 GeneratedMarketSellPostCount = 6;
	int32 MinGeneratedPostItemCount = 1;
	int32 MaxGeneratedPostItemCount = 1;
	int32 MinGeneratedBuyRequestItemCount = 1;
	int32 MaxGeneratedBuyRequestItemCount = 1;
	int32 MinGeneratedSellOfferItemCount = 1;
	int32 MaxGeneratedSellOfferItemCount = 1;
	float MarketBuyRequestPriceMultiplier = 0.75f;
	float MarketSellOfferPriceMultiplier = 1.25f;
	bool bShopDataLoaded = false;
	EFTFlowStateType LastObservedFlowState = EFTFlowStateType::MainMenu;
	FGameplayMessageListenerHandle FlowStateChangedListenerHandle;
};
