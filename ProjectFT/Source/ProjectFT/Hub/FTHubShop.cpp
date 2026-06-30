#include "FTHubShop.h"

#include "ProjectFT/Components/FTInventoryComponent.h"

AFTHubShop::AFTHubShop()
{
	PrimaryActorTick.bCanEverTick = false;

	FTShopItemStruct Floor2Membership;
	Floor2Membership.ItemID = TEXT("2층회원권");
	Floor2Membership.Count = 1;
	Floor2Membership.Price = 0;
	Floor2Membership.bUnlockedByDefault = false;
	Floor2Membership.bFixedSlot = true;
	FixedShopItems.Add(Floor2Membership);

	FTShopItemStruct Floor3Membership;
	Floor3Membership.ItemID = TEXT("3층회원권");
	Floor3Membership.Count = 1;
	Floor3Membership.Price = 0;
	Floor3Membership.bUnlockedByDefault = false;
	Floor3Membership.bFixedSlot = true;
	FixedShopItems.Add(Floor3Membership);

	const TArray<FName> DefaultRandomItemIDs =
	{
		TEXT("물"),
		TEXT("설탕"),
		TEXT("소금"),
		TEXT("비누"),
		TEXT("바게트 빵"),
		TEXT("얼음"),
		TEXT("고무줄"),
		TEXT("자이로볼"),
		TEXT("콜라"),
		TEXT("멘토스"),
		TEXT("토마토"),
		TEXT("드라이어기")
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
	BuyWaterPost.Description = FText::FromString(TEXT("계산대 뒤에서 목이 타는 사람이 있습니다. 상점보다 조금 더 쳐드립니다."));
	BuyWaterPost.ItemID = TEXT("물");
	BuyWaterPost.Count = 1;
	BuyWaterPost.Price = 90;
	BuyWaterPost.bBuyRequest = true;
	MarketBuyPosts.Add(BuyWaterPost);

	FTTradePostStruct BuyColaPost;
	BuyColaPost.PostID = TEXT("Buy_Cola");
	BuyColaPost.Title = FText::FromString(TEXT("[삽니다] 콜라 삽니다"));
	BuyColaPost.Description = FText::FromString(TEXT("멘토스는 제가 준비했습니다. 이유는 묻지 마세요."));
	BuyColaPost.ItemID = TEXT("콜라");
	BuyColaPost.Count = 1;
	BuyColaPost.Price = 140;
	BuyColaPost.bBuyRequest = true;
	MarketBuyPosts.Add(BuyColaPost);

	FTTradePostStruct SellSoapPost;
	SellSoapPost.PostID = TEXT("Sell_Soap");
	SellSoapPost.Title = FText::FromString(TEXT("[팝니다] 거의 새 비누"));
	SellSoapPost.Description = FText::FromString(TEXT("한 번도 안 쓴 것 같은 기분의 비누입니다. 상점보다 쌉니다."));
	SellSoapPost.ItemID = TEXT("비누");
	SellSoapPost.Count = 1;
	SellSoapPost.Price = 70;
	SellSoapPost.bBuyRequest = false;
	MarketSellPosts.Add(SellSoapPost);

	FTTradePostStruct SellMentosPost;
	SellMentosPost.PostID = TEXT("Sell_Mentos");
	SellMentosPost.Title = FText::FromString(TEXT("[팝니다] 멘토스"));
	SellMentosPost.Description = FText::FromString(TEXT("콜라 옆에 두지 않는 조건으로 싸게 넘깁니다."));
	SellMentosPost.ItemID = TEXT("멘토스");
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

	if (!ShopItem || !PlayerInventory || !CanBuyItem(ItemID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Failed: %s"), *ItemID.ToString());
		return false;
	}

	if (!PlayerInventory->AddItem(ShopItem->ItemID, ShopItem->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Reward Failed: %s"), *ItemID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Buy Success: %s x%d"), *ShopItem->ItemID.ToString(), ShopItem->Count);
	return true;
}

bool AFTHubShop::SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory)
{
	if (ItemID.IsNone() || Count <= 0 || !PlayerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s"), *ItemID.ToString());
		return false;
	}

	if (!PlayerInventory->RemoveItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Sell Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Sell Success: %s x%d / Price %d"), *ItemID.ToString(), Count, GetShopSellPrice(ItemID) * Count);
	return true;
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
	if (!Post || !PlayerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Failed: %s"), *PostID.ToString());
		return false;
	}

	if (!PlayerInventory->AddItem(Post->ItemID, Post->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Buy Reward Failed: %s"), *PostID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Buy Success: %s x%d / Price %d"), *Post->ItemID.ToString(), Post->Count, Post->Price);
	return true;
}

bool AFTHubShop::SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	const FTTradePostStruct* Post = FindMarketBuyPost(PostID);
	if (!Post || !PlayerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Failed: %s"), *PostID.ToString());
		return false;
	}

	if (!PlayerInventory->RemoveItem(Post->ItemID, Post->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Market Sell Remove Failed: %s"), *PostID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Market Sell Success: %s x%d / Price %d"), *Post->ItemID.ToString(), Post->Count, Post->Price);
	return true;
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
	return ShopItem && PlayerInventory && IsShopItemUnlocked(ItemID);
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
