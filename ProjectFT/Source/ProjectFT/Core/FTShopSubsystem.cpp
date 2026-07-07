#include "FTShopSubsystem.h"

#include "Engine/AssetManager.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTShopDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"

void UFTShopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
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

bool UFTShopSubsystem::BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory)
{
	EnsureShopDataLoaded();

	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	if (!ShopItem || !CanBuyItem(ItemID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Failed: %s"), *ItemID.ToString());
		return false;
	}

	const int32 Price = FMath::Max(0, ShopItem->Price);
	if (!RemoveCurrency(PlayerInventory, Price))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Currency Failed: %s / Price %d"), *ItemID.ToString(), Price);
		return false;
	}

	const FName ResolvedItemID = ShopItem->GetResolvedItemID();
	if (!PlayerInventory->AddItem(ResolvedItemID, ShopItem->Count))
	{
		AddCurrency(PlayerInventory, Price);
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Reward Failed: %s"), *ItemID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Buy Success: %s x%d / Price %d"), *ResolvedItemID.ToString(), ShopItem->Count, Price);
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

	if (ItemID.IsNone() || ItemID == CurrencyItemID || Count <= 0 || !PlayerInventory)
	{
		return false;
	}

	return GetCombinedItemCount(PlayerInventory, ItemID) >= Count;
}

int32 UFTShopSubsystem::GetShopSellPrice(FName ItemID) const
{
	EnsureShopDataLoaded();

	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	return ShopItem ? FMath::Max(1, ShopItem->Price / 2) : 50;
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
	return true;
}

bool UFTShopSubsystem::CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketSellPost(PostID);
	return Post && PlayerInventory && HasCurrency(PlayerInventory, FMath::Max(0, Post->Price));
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
	return true;
}

bool UFTShopSubsystem::CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	EnsureShopDataLoaded();

	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	const FName ResolvedItemID = Post ? Post->GetResolvedItemID() : NAME_None;
	if (!Post || !PlayerInventory || ResolvedItemID.IsNone() || ResolvedItemID == CurrencyItemID || Post->Count <= 0)
	{
		return false;
	}

	return GetCombinedItemCount(PlayerInventory, ResolvedItemID) >= Post->Count;
}

void UFTShopSubsystem::GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	EnsureShopDataLoaded();
	OutPosts = MarketBuyPosts;
}

void UFTShopSubsystem::GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	EnsureShopDataLoaded();
	OutPosts = MarketSellPosts;
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
	UnlockedShopItemIDs.Reset();
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
		MarketBuyRequestPriceMultiplier = LoadedShopData->MarketBuyRequestPriceMultiplier;
		MarketSellOfferPriceMultiplier = LoadedShopData->MarketSellOfferPriceMultiplier;
		PostPrefixes = LoadedShopData->PostPrefixes;
		BuyRequestReasons = LoadedShopData->BuyRequestReasons;
		SellOfferReasons = LoadedShopData->SellOfferReasons;
		PostEndings = LoadedShopData->PostEndings;
		RandomSlotCount = LoadedShopData->RandomSlotCount;
		CurrencyItemID = LoadedShopData->CurrencyItemID;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Hub shop data asset is not set. Building temporary shop pool from item data assets."));
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

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> ItemAssetIDs;
	AssetManager.GetPrimaryAssetIdList(FName(TEXT("FTItemItem")), ItemAssetIDs);

	TSet<FName> AddedItemIDs;
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
		if (!ItemDataAsset)
		{
			continue;
		}

		const FName ItemID = ItemDataAsset->ItemData.ItemId;
		if (ItemID.IsNone() || ItemID == CurrencyItemID || AddedItemIDs.Contains(ItemID))
		{
			continue;
		}

		OutItemDataAssets.Add(ItemDataAsset);
		AddedItemIDs.Add(ItemID);
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

	UE_LOG(LogTemp, Warning, TEXT("Temporary shop item pool built from item data assets: %d items"), RandomItemPool.Num());
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
	const int32 MinCount = FMath::Max(1, MinGeneratedPostItemCount);
	const int32 MaxCount = FMath::Max(MinCount, MaxGeneratedPostItemCount);
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
	GeneratedPost.Description = FText::FromString(FString::Printf(TEXT("%s x%d. %s. %s"), *ItemName, Count, *Reason.ToString(), *Ending.ToString()));
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
	for (const FTTradePostStruct& Post : MarketSellPosts)
	{
		if (Post.PostID == PostID)
		{
			return &Post;
		}
	}

	return nullptr;
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
