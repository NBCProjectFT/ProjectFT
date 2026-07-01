#include "FTHubShop.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubStorage.h"

AFTHubShop::AFTHubShop()
	: HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	FTShopItemStruct Floor2Membership;
	Floor2Membership.ItemID = TEXT("ID_Common_Floor2_MemberCard");
	Floor2Membership.Count = 1;
	Floor2Membership.Price = 500;
	Floor2Membership.bUnlockedByDefault = false;
	Floor2Membership.bFixedSlot = true;
	FixedShopItems.Add(Floor2Membership);

	const TArray<FName> DefaultRandomItemIDs =
	{
		TEXT("ID_Healing_Water"),
		TEXT("ID_Healing_Sugar"),
		TEXT("ID_Healing_Salt"),
		TEXT("ID_Common_Soap"),
		TEXT("ID_Healing_Baguette"),
		TEXT("ID_Healing_Ice"),
		TEXT("ID_Common_RubberBand"),
		TEXT("ID_Common_Gyroball"),
		TEXT("ID_Healing_Cola"),
		TEXT("ID_Healing_Mentos"),
		TEXT("ID_Healing_Tomato"),
		TEXT("ID_Common_HairDryer")
	};

	for (const FName& ItemID : DefaultRandomItemIDs)
	{
		FTShopItemStruct ShopItem;
		ShopItem.ItemID = ItemID;
		ShopItem.Count = 1;
		ShopItem.Price = 120;
		ShopItem.bUnlockedByDefault = true;
		ShopItem.bFixedSlot = false;
		RandomItemPool.Add(ShopItem);
	}

	FTTradePostStruct BuyWaterPost;
	BuyWaterPost.PostID = TEXT("Buy_Water");
	BuyWaterPost.Title = FText::FromString(TEXT("[삽니다] 물 급구"));
	BuyWaterPost.Description = FText::FromString(TEXT("목이 탄 사람이 있습니다. 상점보다 조금 싸게 받습니다."));
	BuyWaterPost.ItemID = TEXT("ID_Healing_Water");
	BuyWaterPost.Count = 1;
	BuyWaterPost.Price = 90;
	BuyWaterPost.bBuyRequest = true;
	MarketBuyPosts.Add(BuyWaterPost);

	FTTradePostStruct BuyColaPost;
	BuyColaPost.PostID = TEXT("Buy_Cola");
	BuyColaPost.Title = FText::FromString(TEXT("[삽니다] 콜라 삽니다"));
	BuyColaPost.Description = FText::FromString(TEXT("멘토스는 제가 준비했습니다. 이유는 묻지 마세요."));
	BuyColaPost.ItemID = TEXT("ID_Healing_Cola");
	BuyColaPost.Count = 1;
	BuyColaPost.Price = 140;
	BuyColaPost.bBuyRequest = true;
	MarketBuyPosts.Add(BuyColaPost);

	FTTradePostStruct SellSoapPost;
	SellSoapPost.PostID = TEXT("Sell_Soap");
	SellSoapPost.Title = FText::FromString(TEXT("[팝니다] 거의 새 비누"));
	SellSoapPost.Description = FText::FromString(TEXT("한 번도 안 쓴 것 같은 기분의 비누입니다. 상점보다 쌉니다."));
	SellSoapPost.ItemID = TEXT("ID_Common_Soap");
	SellSoapPost.Count = 1;
	SellSoapPost.Price = 70;
	SellSoapPost.bBuyRequest = false;
	MarketSellPosts.Add(SellSoapPost);

	FTTradePostStruct SellMentosPost;
	SellMentosPost.PostID = TEXT("Sell_Mentos");
	SellMentosPost.Title = FText::FromString(TEXT("[팝니다] 멘토스"));
	SellMentosPost.Description = FText::FromString(TEXT("콜라 옆에 두지 않는 조건으로 싸게 넘깁니다."));
	SellMentosPost.ItemID = TEXT("ID_Healing_Mentos");
	SellMentosPost.Count = 1;
	SellMentosPost.Price = 80;
	SellMentosPost.bBuyRequest = false;
	MarketSellPosts.Add(SellMentosPost);
}
void AFTHubShop::BeginPlay()
{
	Super::BeginPlay();

	for (const FTShopItemStruct& ShopItem : FixedShopItems)
	{
		if (ShopItem.bUnlockedByDefault && !ShopItem.ItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ShopItem.ItemID);
		}
	}

	for (const FTShopItemStruct& ShopItem : RandomItemPool)
	{
		if (ShopItem.bUnlockedByDefault && !ShopItem.ItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ShopItem.ItemID);
		}
	}

	RefreshShopItems();
}

bool AFTHubShop::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Hub Shop Interacted"));
	PrintShopItems();
	return true;
}

FText AFTHubShop::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("상점 보기"));
}

void AFTHubShop::RefreshShopItems()
{
	CurrentShopItems.Reset();

	for (const FTShopItemStruct& FixedShopItem : FixedShopItems)
	{
		if (!FixedShopItem.ItemID.IsNone())
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

bool AFTHubShop::BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory)
{
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

	if (!PlayerInventory->AddItem(ShopItem->ItemID, ShopItem->Count))
	{
		AddCurrency(PlayerInventory, Price);
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Reward Failed: %s"), *ItemID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Buy Success: %s x%d / Price %d"), *ShopItem->ItemID.ToString(), ShopItem->Count, Price);
	return true;
}

bool AFTHubShop::SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory)
{
	if (!CanSellItemToShop(ItemID, Count, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	const int32 RewardAmount = GetShopSellPrice(ItemID) * Count;
	if (!PlayerInventory->RemoveItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!AddCurrency(PlayerInventory, RewardAmount))
	{
		PlayerInventory->AddItem(ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Currency Reward Failed: %s x%d / Price %d"), *ItemID.ToString(), Count, RewardAmount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Sell Success: %s x%d / Price %d"), *ItemID.ToString(), Count, RewardAmount);
	return true;
}

bool AFTHubShop::CanSellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory) const
{
	if (ItemID.IsNone() || ItemID == CurrencyItemID || Count <= 0 || !PlayerInventory)
	{
		return false;
	}

	return PlayerInventory->GetItemQuantity(ItemID) >= Count;
}

int32 AFTHubShop::GetShopSellPrice(FName ItemID) const
{
	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	if (ShopItem)
	{
		return FMath::Max(1, ShopItem->Price / 2);
	}

	return 50;
}

bool AFTHubShop::BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
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

	if (!PlayerInventory->AddItem(Post->ItemID, Post->Count))
	{
		AddCurrency(PlayerInventory, Price);
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Reward Failed: %s"), *PostID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Buy Success: %s x%d / Price %d"), *Post->ItemID.ToString(), Post->Count, Price);
	return true;
}

bool AFTHubShop::CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	const FTTradePostStruct* Post = FindMarketSellPost(PostID);
	return Post && PlayerInventory && HasCurrency(PlayerInventory, FMath::Max(0, Post->Price));
}

bool AFTHubShop::SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	if (!Post || !CanSellMarketItem(PostID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Failed: %s"), *PostID.ToString());
		return false;
	}

	if (!PlayerInventory->RemoveItem(Post->ItemID, Post->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Remove Failed: %s"), *PostID.ToString());
		return false;
	}

	const int32 RewardAmount = FMath::Max(0, Post->Price);
	if (!AddCurrency(PlayerInventory, RewardAmount))
	{
		PlayerInventory->AddItem(Post->ItemID, Post->Count);
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Currency Reward Failed: %s / Price %d"), *PostID.ToString(), RewardAmount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Sell Success: %s x%d / Price %d"), *Post->ItemID.ToString(), Post->Count, RewardAmount);
	return true;
}

bool AFTHubShop::CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	if (!Post || !PlayerInventory || Post->ItemID.IsNone() || Post->ItemID == CurrencyItemID || Post->Count <= 0)
	{
		return false;
	}

	return PlayerInventory->GetItemQuantity(Post->ItemID) >= Post->Count;
}

void AFTHubShop::GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	OutPosts = MarketBuyPosts;
}

void AFTHubShop::GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	OutPosts = MarketSellPosts;
}

void AFTHubShop::UnlockShopItem(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return;
	}

	UnlockedShopItemIDs.Add(ItemID);
	UE_LOG(LogTemp, Warning, TEXT("Shop Item Unlocked: %s"), *ItemID.ToString());
}

bool AFTHubShop::IsShopItemUnlocked(FName ItemID) const
{
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
		if (ShopItem.ItemID == ItemID)
		{
			return ShopItem.bUnlockedByDefault;
		}
	}

	return false;
}

bool AFTHubShop::CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const
{
	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	return ShopItem
		&& PlayerInventory
		&& IsShopItemUnlocked(ItemID)
		&& HasCurrency(PlayerInventory, FMath::Max(0, ShopItem->Price));
}

FName AFTHubShop::GetCurrencyItemID() const
{
	return CurrencyItemID;
}

int32 AFTHubShop::GetCurrencyAmount(UFTInventoryComponent* PlayerInventory) const
{
	if (CurrencyItemID.IsNone())
	{
		return 0;
	}

	const int32 PlayerCurrencyAmount = PlayerInventory
		? PlayerInventory->GetItemQuantity(CurrencyItemID)
		: 0;
	const int32 StorageCurrencyAmount = HubStorage
		? HubStorage->GetStorageItemCount(CurrencyItemID)
		: 0;

	return PlayerCurrencyAmount + StorageCurrencyAmount;
}

void AFTHubShop::GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const
{
	OutShopItems = CurrentShopItems;
}

const FTShopItemStruct* AFTHubShop::FindCurrentShopItem(FName ItemID) const
{
	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		if (ShopItem.ItemID == ItemID)
		{
			return &ShopItem;
		}
	}

	return nullptr;
}

const FTTradePostStruct* AFTHubShop::FindMarketBuyPost(FName PostID) const
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

const FTTradePostStruct* AFTHubShop::FindMarketSellPost(FName PostID) const
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

bool AFTHubShop::HasCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
{
	if (Amount <= 0)
	{
		return true;
	}

	return !CurrencyItemID.IsNone() && GetCurrencyAmount(PlayerInventory) >= Amount;
}

bool AFTHubShop::AddCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
{
	if (Amount <= 0)
	{
		return true;
	}

	return PlayerInventory && !CurrencyItemID.IsNone() && PlayerInventory->AddItem(CurrencyItemID, Amount);
}

bool AFTHubShop::RemoveCurrency(UFTInventoryComponent* PlayerInventory, int32 Amount) const
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
		if (!HubStorage || !HubStorage->RemoveStorageItem(CurrencyItemID, RemainingAmount))
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

void AFTHubShop::PrintShopItems() const
{
	if (CurrentShopItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Empty"));
		return;
	}

	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shop: %s x%d / Price %d / Unlocked %s"),
			*ShopItem.ItemID.ToString(),
			ShopItem.Count,
			ShopItem.Price,
			IsShopItemUnlocked(ShopItem.ItemID) ? TEXT("true") : TEXT("false")
		);
	}
}
