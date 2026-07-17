#include "FTShopSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTGameFlowSubsystem.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Data/FTInventoryPreloadDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTLevelPreloadDataAsset.h"
#include "ProjectFT/Data/FTShopDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

namespace
{
	const FPrimaryAssetType ItemAssetType(TEXT("FTItemItem"));
	const FName ItemDataPackagePath(TEXT("/Game/Blueprints/Items/Data"));
}

void UFTShopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FlowStateChangedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_FlowStateChanged,
		this,
		&ThisClass::HandleFlowStateChanged);
}

void UFTShopSubsystem::Deinitialize()
{
	if (FlowStateChangedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(FlowStateChangedListenerHandle);
		FlowStateChangedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::Deinitialize();
}

void UFTShopSubsystem::ConfigureHubStorage(AFTHubStorage* InHubStorage)
{
	HubStorage = InHubStorage;
	EnsureShopDataLoaded();
}

void UFTShopSubsystem::RefreshShopItems()
{
	EnsureShopDataLoaded();

	CurrentShopItems.Reset();

	for (const FTShopItemStruct& FixedShopItem : FixedShopItems)
	{
		if (!FixedShopItem.GetResolvedItemID().IsNone())
		{
			CurrentShopItems.Add(FixedShopItem);
		}
	}

	TArray<FTShopItemStruct> CandidateItems = RandomItemPool;
	const int32 SlotCount = FMath::Max(0, RandomSlotCount);
	for (int32 Index = 0; Index < SlotCount && CandidateItems.Num() > 0; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateItems.Num() - 1);
		CurrentShopItems.Add(CandidateItems[RandomIndex]);
		CandidateItems.RemoveAt(RandomIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Items Refreshed: %d items"), CurrentShopItems.Num());
}

void UFTShopSubsystem::RefreshShopAndMarketListings()
{
	EnsureShopDataLoaded();

	RefreshShopItems();
	ConsumedMarketPostIDs.Reset();

	if (bGenerateMarketPostsFromTemplates)
	{
		GenerateMarketPostsFromTemplates();
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Shop and market listings refreshed after returning to hub. ShopItems=%d MarketBuyPosts=%d MarketSellPosts=%d"),
		CurrentShopItems.Num(),
		MarketBuyPosts.Num(),
		MarketSellPosts.Num());
}

void UFTShopSubsystem::HandleFlowStateChanged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	const EFTFlowStateType NewFlowState = static_cast<EFTFlowStateType>(FMath::RoundToInt(Payload.Value));
	const bool bReturnedToBaseFromRaid = NewFlowState == EFTFlowStateType::Base && WasRaidFlowState(LastObservedFlowState);
	LastObservedFlowState = NewFlowState;

	if (bReturnedToBaseFromRaid)
	{
		RefreshShopAndMarketListings();
	}
}

bool UFTShopSubsystem::WasRaidFlowState(const EFTFlowStateType FlowState) const
{
	return FlowState == EFTFlowStateType::RaidEntering
		|| FlowState == EFTFlowStateType::RaidInProgress
		|| FlowState == EFTFlowStateType::Escaping
		|| FlowState == EFTFlowStateType::Escaped
		|| FlowState == EFTFlowStateType::Failed;
}

bool UFTShopSubsystem::BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory)
{
	return BuyItemCount(ItemID, 1, PlayerInventory);
}

bool UFTShopSubsystem::BuyItemCount(FName ItemID, const int32 PurchaseCount, UFTInventoryComponent* PlayerInventory)
{
	EnsureShopDataLoaded();

	const int32 SafePurchaseCount = FMath::Max(0, PurchaseCount);
	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	if (!ShopItem || SafePurchaseCount <= 0 || !CanBuyItem(ItemID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Failed: %s x%d"), *ItemID.ToString(), PurchaseCount);
		return false;
	}

	const int32 Price = FMath::Max(0, ShopItem->Price) * SafePurchaseCount;
	if (!RemoveCurrency(PlayerInventory, Price))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Currency Failed: %s / Price %d"), *ItemID.ToString(), Price);
		return false;
	}

	const FName ResolvedItemID = ShopItem->GetResolvedItemID();
	const int32 RewardCount = ShopItem->Count * SafePurchaseCount;
	if (!PlayerInventory->AddItem(ResolvedItemID, RewardCount))
	{
		AddCurrency(PlayerInventory, Price);
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Reward Failed: %s x%d"), *ItemID.ToString(), RewardCount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Buy Success: %s x%d / Price %d"), *ResolvedItemID.ToString(), RewardCount, Price);
	return true;
}

bool UFTShopSubsystem::SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory)
{
	EnsureShopDataLoaded();

	if (!CanSellItemToShop(ItemID, Count, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	const int32 RewardAmount = GetShopSellPrice(ItemID) * Count;
	if (!ConsumeCombinedItem(PlayerInventory, ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!AddCurrency(PlayerInventory, RewardAmount))
	{
		// Rewards and rollback both return to the player inventory.
		PlayerInventory->AddItem(ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Currency Reward Failed: %s x%d / Price %d"), *ItemID.ToString(), Count, RewardAmount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Sell Success: %s x%d / Price %d"), *ItemID.ToString(), Count, RewardAmount);
	return true;
}

bool UFTShopSubsystem::CanSellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	if (!IsItemSellableToShop(ItemID) || Count <= 0 || !PlayerInventory)
	{
		return false;
	}

	return GetCombinedItemCount(PlayerInventory, ItemID) >= Count;
}

int32 UFTShopSubsystem::GetShopSellPrice(FName ItemID) const
{
	EnsureShopDataLoaded();
	if (ItemID.IsNone() || ItemID == CurrencyItemID || SellExcludedItemIDs.Contains(ItemID))
	{
		return 0;
	}

	if (const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID))
	{
		return FMath::Max(1, ShopItem->Price / 2);
	}

	const UFTItemDataAsset* ItemDataAsset = UFTItemFunctionLibrary::FindItemData(this, ItemID);
	return ItemDataAsset
		? FMath::Max(1, ItemDataAsset->ItemData.Cost / 2)
		: 0;
}

bool UFTShopSubsystem::IsItemSellableToShop(FName ItemID) const
{
	return GetShopSellPrice(ItemID) > 0;
}

bool UFTShopSubsystem::BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketSellPost(PostID);
	if (!Post || !CanBuyMarketItem(PostID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Failed: %s"), *PostID.ToString());
		return false;
	}

	const int32 Price = FMath::Max(0, Post->Price);
	if (!RemoveCurrency(PlayerInventory, Price))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Currency Failed: %s / Price %d"), *PostID.ToString(), Price);
		return false;
	}

	const FName ResolvedItemID = Post->GetResolvedItemID();
	if (!PlayerInventory->AddItem(ResolvedItemID, Post->Count))
	{
		AddCurrency(PlayerInventory, Price);
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Reward Failed: %s"), *PostID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Buy Success: %s x%d / Price %d"), *ResolvedItemID.ToString(), Post->Count, Price);
	ConsumedMarketPostIDs.Add(PostID);
	return true;
}

bool UFTShopSubsystem::CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketSellPost(PostID);
	return Post
		&& PlayerInventory
		&& !Post->GetResolvedItemID().IsNone()
		&& Post->Count > 0
		&& IsMarketPostCountInRange(*Post, false)
		&& HasCurrency(PlayerInventory, FMath::Max(0, Post->Price));
}

bool UFTShopSubsystem::SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	if (!Post || !CanSellMarketItem(PostID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Failed: %s"), *PostID.ToString());
		return false;
	}

	const FName ResolvedItemID = Post->GetResolvedItemID();
	if (!ConsumeCombinedItem(PlayerInventory, ResolvedItemID, Post->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Remove Failed: %s"), *PostID.ToString());
		return false;
	}

	const int32 RewardAmount = FMath::Max(0, Post->Price);
	if (!AddCurrency(PlayerInventory, RewardAmount))
	{
		PlayerInventory->AddItem(ResolvedItemID, Post->Count);
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Currency Reward Failed: %s / Price %d"), *PostID.ToString(), RewardAmount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Sell Success: %s x%d / Price %d"), *ResolvedItemID.ToString(), Post->Count, RewardAmount);
	ConsumedMarketPostIDs.Add(PostID);
	return true;
}

bool UFTShopSubsystem::CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	const FName ResolvedItemID = Post ? Post->GetResolvedItemID() : NAME_None;
	if (!Post || !PlayerInventory || ResolvedItemID.IsNone() || ResolvedItemID == CurrencyItemID || Post->Count <= 0 || !IsMarketPostCountInRange(*Post, true))
	{
		return false;
	}

	return GetCombinedItemCount(PlayerInventory, ResolvedItemID) >= Post->Count;
}

void UFTShopSubsystem::GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	EnsureShopDataLoaded();
	OutPosts.Reset();
	for (const FTTradePostStruct& Post : MarketBuyPosts)
	{
		if (!IsMarketPostConsumed(Post.PostID))
		{
			OutPosts.Add(Post);
		}
	}
}

void UFTShopSubsystem::GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	EnsureShopDataLoaded();
	OutPosts.Reset();
	for (const FTTradePostStruct& Post : MarketSellPosts)
	{
		if (!IsMarketPostConsumed(Post.PostID))
		{
			OutPosts.Add(Post);
		}
	}
}

void UFTShopSubsystem::UnlockShopItem(FName ItemID)
{
	EnsureShopDataLoaded();

	if (ItemID.IsNone())
	{
		return;
	}

	UnlockedShopItemIDs.Add(ItemID);
	UE_LOG(LogTemp, Warning, TEXT("Shop Item Unlocked: %s"), *ItemID.ToString());
}

bool UFTShopSubsystem::IsShopItemUnlocked(FName ItemID) const
{
	EnsureShopDataLoaded();

	if (ItemID.IsNone())
	{
		return false;
	}

	if (UnlockedShopItemIDs.Contains(ItemID))
	{
		return true;
	}

	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		if (ShopItem.GetResolvedItemID() == ItemID)
		{
			return ShopItem.bUnlockedByDefault;
		}
	}

	return false;
}

bool UFTShopSubsystem::CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	return ShopItem
		&& PlayerInventory
		&& IsShopItemUnlocked(ItemID)
		&& HasCurrency(PlayerInventory, FMath::Max(0, ShopItem->Price));
}

FName UFTShopSubsystem::GetCurrencyItemID() const
{
	EnsureShopDataLoaded();
	return CurrencyItemID;
}

int32 UFTShopSubsystem::GetCurrencyAmount(UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	if (CurrencyItemID.IsNone())
	{
		return 0;
	}

	const int32 PlayerCurrencyAmount = PlayerInventory
		? PlayerInventory->GetItemQuantity(CurrencyItemID)
		: 0;
	const UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;
	const int32 StorageCurrencyAmount = StorageSubsystem
		? StorageSubsystem->GetStorageItemCount(HubStorage ? HubStorage->GetStorageInventory() : nullptr, CurrencyItemID)
		: 0;

	return PlayerCurrencyAmount + StorageCurrencyAmount;
}

void UFTShopSubsystem::GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const
{
	EnsureShopDataLoaded();
	OutShopItems = CurrentShopItems;
}

void UFTShopSubsystem::EnsureShopDataLoaded() const
{
	if (!bShopDataLoaded)
	{
		const_cast<UFTShopSubsystem*>(this)->LoadShopData();
	}
}

void UFTShopSubsystem::LoadShopData()
{
	FixedShopItems.Reset();
	RandomItemPool.Reset();
	MarketBuyPosts.Reset();
	MarketSellPosts.Reset();
	ConsumedMarketPostIDs.Reset();
	UnlockedShopItemIDs.Reset();
	SellExcludedItemIDs.Reset();
	PostPrefixes.Reset();
	BuyRequestReasons.Reset();
	SellOfferReasons.Reset();
	PostEndings.Reset();

	const UFTShopDataAsset* LoadedShopData = ResolveShopDataAsset();
	if (LoadedShopData)
	{
		FixedShopItems = LoadedShopData->FixedShopItems;
		RandomItemPool = LoadedShopData->RandomItemPool;
		MarketBuyPosts = LoadedShopData->MarketBuyPosts;
		MarketSellPosts = LoadedShopData->MarketSellPosts;
		bGenerateMarketPostsFromTemplates = LoadedShopData->bGenerateMarketPostsFromTemplates;
		GeneratedMarketBuyPostCount = LoadedShopData->GeneratedMarketBuyPostCount;
		GeneratedMarketSellPostCount = LoadedShopData->GeneratedMarketSellPostCount;
		MinGeneratedPostItemCount = LoadedShopData->MinGeneratedPostItemCount;
		MaxGeneratedPostItemCount = LoadedShopData->MaxGeneratedPostItemCount;
		MinGeneratedBuyRequestItemCount = LoadedShopData->MinGeneratedBuyRequestItemCount;
		MaxGeneratedBuyRequestItemCount = LoadedShopData->MaxGeneratedBuyRequestItemCount;
		MinGeneratedSellOfferItemCount = LoadedShopData->MinGeneratedSellOfferItemCount;
		MaxGeneratedSellOfferItemCount = LoadedShopData->MaxGeneratedSellOfferItemCount;
		MarketBuyRequestPriceMultiplier = LoadedShopData->MarketBuyRequestPriceMultiplier;
		MarketSellOfferPriceMultiplier = LoadedShopData->MarketSellOfferPriceMultiplier;
		PostPrefixes = LoadedShopData->PostPrefixes;
		BuyRequestReasons = LoadedShopData->BuyRequestReasons;
		SellOfferReasons = LoadedShopData->SellOfferReasons;
		PostEndings = LoadedShopData->PostEndings;
		RandomSlotCount = LoadedShopData->RandomSlotCount;
		CurrencyItemID = LoadedShopData->CurrencyItemID;

		for (const TSoftObjectPtr<UFTItemDataAsset>& ExcludedItemRef : LoadedShopData->SellExcludedItems)
		{
			if (const UFTItemDataAsset* ExcludedItem = ExcludedItemRef.LoadSynchronous())
			{
				if (!ExcludedItem->ItemData.ItemId.IsNone())
				{
					SellExcludedItemIDs.Add(ExcludedItem->ItemData.ItemId);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hub shop data asset is not set. Building fallback shop pool from item data assets."));
	}

	if (!CurrencyItemID.IsNone())
	{
		SellExcludedItemIDs.Add(CurrencyItemID);
	}

	if (RandomItemPool.IsEmpty())
	{
		BuildRandomItemPoolFromItemAssets();
	}

	if (bGenerateMarketPostsFromTemplates)
	{
		GenerateMarketPostsFromTemplates();
	}

	for (const FTShopItemStruct& ShopItem : FixedShopItems)
	{
		const FName ResolvedItemID = ShopItem.GetResolvedItemID();
		if (ShopItem.bUnlockedByDefault && !ResolvedItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ResolvedItemID);
		}
	}

	for (const FTShopItemStruct& ShopItem : RandomItemPool)
	{
		const FName ResolvedItemID = ShopItem.GetResolvedItemID();
		if (ShopItem.bUnlockedByDefault && !ResolvedItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ResolvedItemID);
		}
	}

	bShopDataLoaded = true;
	RefreshShopItems();
}

const UFTShopDataAsset* UFTShopSubsystem::ResolveShopDataAsset() const
{
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		return UFTAssetManager::GetAsset(GameData->HubShopDataAsset);
	}

	return nullptr;
}

void UFTShopSubsystem::CollectItemDataAssets(TArray<UFTItemDataAsset*>& OutItemDataAssets) const
{
	OutItemDataAssets.Reset();

	TSet<FName> AddedItemIDs;
	const auto TryAddItemDataAsset = [this, &OutItemDataAssets, &AddedItemIDs](UFTItemDataAsset* ItemDataAsset)
	{
		if (!ItemDataAsset)
		{
			return false;
		}

		const FName ItemID = ItemDataAsset->ItemData.ItemId;
		if (ItemID.IsNone() || ItemID == CurrencyItemID || AddedItemIDs.Contains(ItemID))
		{
			return false;
		}

		OutItemDataAssets.Add(ItemDataAsset);
		AddedItemIDs.Add(ItemID);
		return true;
	};

	UAssetManager& AssetManager = UAssetManager::Get();
	const auto LoadItemDataFromPath = [&TryAddItemDataAsset](const FSoftObjectPath& AssetPath)
	{
		if (!AssetPath.IsValid())
		{
			return false;
		}

		UObject* AssetObject = AssetPath.ResolveObject();
		if (!AssetObject)
		{
			AssetObject = AssetPath.TryLoad();
		}

		return TryAddItemDataAsset(Cast<UFTItemDataAsset>(AssetObject));
	};

	const UFTLevelPreloadDataAsset* LevelPreloadData = nullptr;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFTGameFlowSubsystem* FlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			const TSoftObjectPtr<UFTLevelPreloadDataAsset> LevelPreloadRef =
				FlowSubsystem->GetLevelPreloadDataAssetForState(EFTFlowStateType::Base);
			LevelPreloadData = UFTAssetManager::GetAsset(LevelPreloadRef);
		}
	}

	const UFTInventoryPreloadDataAsset* InventoryPreloadData = nullptr;
	if (LevelPreloadData && LevelPreloadData->bUseInventoryPreloadDataAsset)
	{
		InventoryPreloadData = UFTAssetManager::GetAsset(LevelPreloadData->InventoryPreloadDataAsset);
	}

	if (InventoryPreloadData)
	{
		const int32 BeforePreloadCollectCount = OutItemDataAssets.Num();
		TArray<FSoftObjectPath> PreloadAssetPaths;
		InventoryPreloadData->GetPreloadAssetPaths(PreloadAssetPaths);
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shop item data collect: using LevelPreload=%s InventoryPreload=%s includeAllPrimaryItems=%s explicitPaths=%d"),
			*GetNameSafe(LevelPreloadData),
			*GetNameSafe(InventoryPreloadData),
			InventoryPreloadData->bIncludeAllPrimaryItemAssets ? TEXT("true") : TEXT("false"),
			PreloadAssetPaths.Num());

		if (InventoryPreloadData->bIncludeAllPrimaryItemAssets)
		{
			TArray<FString> ItemDataPaths;
			ItemDataPaths.Add(ItemDataPackagePath.ToString());
			AssetManager.ScanPathsForPrimaryAssets(ItemAssetType, ItemDataPaths, UFTItemDataAsset::StaticClass(), false, false, true);

			TSet<FString> AddedPreloadPathStrings;
			for (const FSoftObjectPath& PreloadAssetPath : PreloadAssetPaths)
			{
				if (PreloadAssetPath.IsValid())
				{
					AddedPreloadPathStrings.Add(PreloadAssetPath.ToString());
				}
			}

			TArray<FPrimaryAssetId> ItemAssetIDs;
			AssetManager.GetPrimaryAssetIdList(ItemAssetType, ItemAssetIDs);
			int32 AppendedPrimaryItemCount = 0;
			for (const FPrimaryAssetId& ItemAssetID : ItemAssetIDs)
			{
				const FSoftObjectPath ItemAssetPath = AssetManager.GetPrimaryAssetPath(ItemAssetID);
				if (ItemAssetPath.IsValid() && !AddedPreloadPathStrings.Contains(ItemAssetPath.ToString()))
				{
					PreloadAssetPaths.Add(ItemAssetPath);
					AddedPreloadPathStrings.Add(ItemAssetPath.ToString());
					++AppendedPrimaryItemCount;
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: InventoryPreload appended primary item paths=%d"), AppendedPrimaryItemCount);
		}

		TSet<FString> ExcludedPathStrings;
		InventoryPreloadData->GetExcludedAssetPaths(ExcludedPathStrings);
		const int32 BeforeExcludePathCount = PreloadAssetPaths.Num();
		PreloadAssetPaths.RemoveAll([&ExcludedPathStrings](const FSoftObjectPath& PreloadAssetPath)
		{
			return ExcludedPathStrings.Contains(PreloadAssetPath.ToString());
		});
		if (BeforeExcludePathCount != PreloadAssetPaths.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: InventoryPreload excluded paths=%d"), BeforeExcludePathCount - PreloadAssetPaths.Num());
		}

		int32 AddedFromPreloadCount = 0;
		int32 SkippedFromPreloadCount = 0;
		for (const FSoftObjectPath& PreloadAssetPath : PreloadAssetPaths)
		{
			if (LoadItemDataFromPath(PreloadAssetPath))
			{
				++AddedFromPreloadCount;
			}
			else
			{
				++SkippedFromPreloadCount;
			}
		}
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shop item data collect: InventoryPreload result added=%d skipped=%d totalCollected=%d"),
			AddedFromPreloadCount,
			SkippedFromPreloadCount,
			OutItemDataAssets.Num() - BeforePreloadCollectCount);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shop item data collect: Base LevelPreload has no usable InventoryPreload reference. LevelPreload=%s"),
			*GetNameSafe(LevelPreloadData));
	}
	UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: InventoryPreload collected=%d"), OutItemDataAssets.Num());

	if (OutItemDataAssets.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: InventoryPreload produced no usable items. Falling back to AssetManager item primary assets."));

		TArray<FString> ItemDataPaths;
		ItemDataPaths.Add(ItemDataPackagePath.ToString());
		AssetManager.ScanPathsForPrimaryAssets(ItemAssetType, ItemDataPaths, UFTItemDataAsset::StaticClass(), false, false, true);

		TArray<FPrimaryAssetId> ItemAssetIDs;
		AssetManager.GetPrimaryAssetIdList(ItemAssetType, ItemAssetIDs);
		UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: AssetManager fallback primary ids=%d"), ItemAssetIDs.Num());

		for (const FPrimaryAssetId& ItemAssetID : ItemAssetIDs)
		{
			UObject* AssetObject = AssetManager.GetPrimaryAssetObject(ItemAssetID);
			if (!AssetObject)
			{
				const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ItemAssetID);
				if (AssetPath.IsValid())
				{
					AssetObject = AssetPath.TryLoad();
				}
			}

			UFTItemDataAsset* ItemDataAsset = Cast<UFTItemDataAsset>(AssetObject);
			TryAddItemDataAsset(ItemDataAsset);
		}
		UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: AssetManager fallback collected=%d"), OutItemDataAssets.Num());
	}

	if (OutItemDataAssets.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: using AssetRegistry fallback path=%s"), *ItemDataPackagePath.ToString());

		TArray<FString> ItemDataPaths;
		ItemDataPaths.Add(ItemDataPackagePath.ToString());

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		AssetRegistry.ScanPathsSynchronous(ItemDataPaths, true);

		FARFilter Filter;
		Filter.PackagePaths.Add(ItemDataPackagePath);
		Filter.ClassPaths.Add(UFTItemDataAsset::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;

		TArray<FAssetData> ItemAssetDataList;
		AssetRegistry.GetAssets(Filter, ItemAssetDataList);

		for (const FAssetData& ItemAssetData : ItemAssetDataList)
		{
			TryAddItemDataAsset(Cast<UFTItemDataAsset>(ItemAssetData.GetAsset()));
		}
		UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: AssetRegistry assets=%d collected=%d"), ItemAssetDataList.Num(), OutItemDataAssets.Num());
	}

	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		const int32 BeforeGameDataCount = OutItemDataAssets.Num();
		for (const TSoftObjectPtr<UFTItemDataAsset>& ItemDataAssetRef : GameData->ItemDataAssets)
		{
			TryAddItemDataAsset(UFTAssetManager::GetAsset(ItemDataAssetRef));
		}
		if (OutItemDataAssets.Num() > BeforeGameDataCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shop item data collect: GameData fallback appended=%d total=%d"), OutItemDataAssets.Num() - BeforeGameDataCount, OutItemDataAssets.Num());
		}
	}
}

void UFTShopSubsystem::BuildRandomItemPoolFromItemAssets()
{
	TArray<UFTItemDataAsset*> ItemDataAssets;
	CollectItemDataAssets(ItemDataAssets);

	for (UFTItemDataAsset* ItemDataAsset : ItemDataAssets)
	{
		if (!ItemDataAsset)
		{
			continue;
		}

		FTShopItemStruct ShopItem;
		ShopItem.ItemDataAsset = ItemDataAsset;
		ShopItem.ItemID = ItemDataAsset->ItemData.ItemId;
		ShopItem.Count = 1;
		ShopItem.Price = FMath::Max(1, ItemDataAsset->ItemData.Cost);
		ShopItem.bUnlockedByDefault = true;
		ShopItem.bFixedSlot = false;
		RandomItemPool.Add(ShopItem);
	}

	UE_LOG(LogTemp, Warning, TEXT("Fallback shop item pool built from item data assets: %d items"), RandomItemPool.Num());
}

void UFTShopSubsystem::GenerateMarketPostsFromTemplates()
{
	MarketBuyPosts.Reset();
	MarketSellPosts.Reset();

	TArray<UFTItemDataAsset*> CandidateItems;
	CollectItemDataAssets(CandidateItems);
	if (CandidateItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Market post generation skipped because no item data assets were found."));
		return;
	}

	const int32 BuyPostCount = FMath::Max(0, GeneratedMarketBuyPostCount);
	for (int32 Index = 0; Index < BuyPostCount; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateItems.Num() - 1);
		MarketBuyPosts.Add(BuildGeneratedMarketPost(*CandidateItems[RandomIndex], Index, true));
	}

	const int32 SellPostCount = FMath::Max(0, GeneratedMarketSellPostCount);
	for (int32 Index = 0; Index < SellPostCount; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateItems.Num() - 1);
		MarketSellPosts.Add(BuildGeneratedMarketPost(*CandidateItems[RandomIndex], Index, false));
	}

	UE_LOG(LogTemp, Warning, TEXT("Generated market posts from templates. BuyRequests: %d / SellOffers: %d"), MarketBuyPosts.Num(), MarketSellPosts.Num());
}

FTTradePostStruct UFTShopSubsystem::BuildGeneratedMarketPost(UFTItemDataAsset& ItemDataAsset, int32 PostIndex, bool bBuyRequest) const
{
	const FName ItemID = ItemDataAsset.ItemData.ItemId;
	const FString ItemName = ItemDataAsset.ItemData.ItemName.IsEmpty()
		? ItemID.ToString()
		: ItemDataAsset.ItemData.ItemName.ToString();
	const int32 MinCount = FMath::Max(1, bBuyRequest ? MinGeneratedBuyRequestItemCount : MinGeneratedSellOfferItemCount);
	const int32 MaxCount = FMath::Max(MinCount, bBuyRequest ? MaxGeneratedBuyRequestItemCount : MaxGeneratedSellOfferItemCount);
	const int32 Count = FMath::RandRange(MinCount, MaxCount);
	const float PriceMultiplier = bBuyRequest ? MarketBuyRequestPriceMultiplier : MarketSellOfferPriceMultiplier;
	const int32 UnitPrice = FMath::Max(1, FMath::RoundToInt(FMath::Max(1, ItemDataAsset.ItemData.Cost) * FMath::Max(0.0f, PriceMultiplier)));

	const FText Prefix = PickTemplateText(PostPrefixes, FText::FromString(TEXT("거래")));
	const FText Reason = bBuyRequest
		? PickTemplateText(BuyRequestReasons, FText::FromString(TEXT("필요해서 구해봅니다")))
		: PickTemplateText(SellOfferReasons, FText::FromString(TEXT("안 써서 정리합니다")));
	const FText Ending = PickTemplateText(PostEndings, FText::FromString(TEXT("연락주세요")));

	FTTradePostStruct GeneratedPost;
	GeneratedPost.PostID = FName(*FString::Printf(TEXT("Generated_%s_%d_%s"), bBuyRequest ? TEXT("Buy") : TEXT("Sell"), PostIndex, *ItemID.ToString()));
	GeneratedPost.Title = FText::FromString(FString::Printf(TEXT("%s %s"), *Prefix.ToString(), *ItemName));
	GeneratedPost.Description = FText::FromString(FString::Printf(TEXT("%s. %s"), *Reason.ToString(), *Ending.ToString()));
	GeneratedPost.ItemDataAsset = &ItemDataAsset;
	GeneratedPost.ItemID = ItemID;
	GeneratedPost.Count = Count;
	GeneratedPost.Price = UnitPrice * Count;
	GeneratedPost.bBuyRequest = bBuyRequest;
	return GeneratedPost;
}

FText UFTShopSubsystem::PickTemplateText(const TArray<FText>& Templates, const FText& FallbackText) const
{
	if (Templates.IsEmpty())
	{
		return FallbackText;
	}

	return Templates[FMath::RandRange(0, Templates.Num() - 1)];
}

bool UFTShopSubsystem::IsMarketPostConsumed(const FName PostID) const
{
	return !PostID.IsNone() && ConsumedMarketPostIDs.Contains(PostID);
}

const FTShopItemStruct* UFTShopSubsystem::FindCurrentShopItem(FName ItemID) const
{
	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		if (ShopItem.GetResolvedItemID() == ItemID)
		{
			return &ShopItem;
		}
	}

	return nullptr;
}

const FTTradePostStruct* UFTShopSubsystem::FindMarketBuyPost(FName PostID) const
{
	if (IsMarketPostConsumed(PostID))
	{
		return nullptr;
	}

	for (const FTTradePostStruct& Post : MarketBuyPosts)
	{
		if (Post.PostID == PostID)
		{
			return &Post;
		}
	}

	return nullptr;
}

const FTTradePostStruct* UFTShopSubsystem::FindMarketSellPost(FName PostID) const
{
	if (IsMarketPostConsumed(PostID))
	{
		return nullptr;
	}

	for (const FTTradePostStruct& Post : MarketSellPosts)
	{
		if (Post.PostID == PostID)
		{
			return &Post;
		}
	}

	return nullptr;
}

bool UFTShopSubsystem::IsMarketPostCountInRange(const FTTradePostStruct& Post, const bool bBuyRequest) const
{
	const int32 MinCount = FMath::Max(1, bBuyRequest ? MinGeneratedBuyRequestItemCount : MinGeneratedSellOfferItemCount);
	const int32 MaxCount = FMath::Max(MinCount, bBuyRequest ? MaxGeneratedBuyRequestItemCount : MaxGeneratedSellOfferItemCount);
	return Post.Count >= MinCount && Post.Count <= MaxCount;
}

int32 UFTShopSubsystem::GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const
{
	const UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	return StorageSubsystem
		? StorageSubsystem->GetCombinedItemCount(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID)
		: (PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0);
}

bool UFTShopSubsystem::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count) const
{
	UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;

	if (StorageSubsystem)
	{
		return StorageSubsystem->ConsumeCombinedItem(PlayerInventory, HubStorage ? HubStorage->GetStorageInventory() : nullptr, ItemID, Count);
	}

	return PlayerInventory && !ItemID.IsNone() && Count > 0 && PlayerInventory->RemoveItem(ItemID, Count);
}

bool UFTShopSubsystem::HasCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
{
	if (Amount <= 0)
	{
		return true;
	}

	return !CurrencyItemID.IsNone() && GetCurrencyAmount(PlayerInventory) >= Amount;
}

bool UFTShopSubsystem::AddCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
{
	if (Amount <= 0)
	{
		return true;
	}

	return PlayerInventory && !CurrencyItemID.IsNone() && PlayerInventory->AddItem(CurrencyItemID, Amount);
}

bool UFTShopSubsystem::RemoveCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
{
	if (Amount <= 0)
	{
		return true;
	}

	if (CurrencyItemID.IsNone() || GetCurrencyAmount(PlayerInventory) < Amount)
	{
		return false;
	}

	int32 RemainingAmount = Amount;
	int32 RemovedFromPlayer = 0;

	if (PlayerInventory)
	{
		const int32 PlayerCurrencyAmount = PlayerInventory->GetItemQuantity(CurrencyItemID);
		RemovedFromPlayer = FMath::Min(PlayerCurrencyAmount, RemainingAmount);
		if (RemovedFromPlayer > 0 && !PlayerInventory->RemoveItem(CurrencyItemID, RemovedFromPlayer))
		{
			return false;
		}

		RemainingAmount -= RemovedFromPlayer;
	}

	if (RemainingAmount > 0)
	{
		UFTStorageSubsystem* StorageSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
			: nullptr;

		if (!StorageSubsystem || !StorageSubsystem->RemoveStorageItem(HubStorage ? HubStorage->GetStorageInventory() : nullptr, CurrencyItemID, RemainingAmount))
		{
			if (RemovedFromPlayer > 0 && PlayerInventory)
			{
				PlayerInventory->AddItem(CurrencyItemID, RemovedFromPlayer);
			}

			return false;
		}
	}

	return true;
}
