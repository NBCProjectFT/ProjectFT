#include "FTHubShop.h"

#include "ProjectFT/Core/FTShopSubsystem.h"

AFTHubShop::AFTHubShop()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTHubShop::BeginPlay()
{
	Super::BeginPlay();

	if (UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->ConfigureHubStorage(HubStorage);
	}
}

bool AFTHubShop::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("AFTHubShop is deprecated. Hub shop logic is handled by UFTShopSubsystem."));
	return true;
}

FText AFTHubShop::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("상점 보기"));
}

void AFTHubShop::RefreshShopItems()
{
	if (UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->RefreshShopItems();
	}
}

bool AFTHubShop::BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory)
{
	UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->BuyItem(ItemID, PlayerInventory);
}

bool AFTHubShop::SellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory)
{
	UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->SellItemToShop(ItemID, Count, PlayerInventory);
}

bool AFTHubShop::CanSellItemToShop(FName ItemID, int32 Count, UFTInventoryComponent* PlayerInventory) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->CanSellItemToShop(ItemID, Count, PlayerInventory);
}

int32 AFTHubShop::GetShopSellPrice(FName ItemID) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem ? ShopSubsystem->GetShopSellPrice(ItemID) : 0;
}

bool AFTHubShop::BuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->BuyMarketItem(PostID, PlayerInventory);
}

bool AFTHubShop::CanBuyMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->CanBuyMarketItem(PostID, PlayerInventory);
}

bool AFTHubShop::SellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory)
{
	UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->SellMarketItem(PostID, PlayerInventory);
}

bool AFTHubShop::CanSellMarketItem(FName PostID, UFTInventoryComponent* PlayerInventory) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->CanSellMarketItem(PostID, PlayerInventory);
}

void AFTHubShop::GetMarketBuyPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	if (const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->GetMarketBuyPosts(OutPosts);
	}
}

void AFTHubShop::GetMarketSellPosts(TArray<FTTradePostStruct>& OutPosts) const
{
	if (const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->GetMarketSellPosts(OutPosts);
	}
}

void AFTHubShop::UnlockShopItem(FName ItemID)
{
	if (UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->UnlockShopItem(ItemID);
	}
}

bool AFTHubShop::IsShopItemUnlocked(FName ItemID) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->IsShopItemUnlocked(ItemID);
}

bool AFTHubShop::CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem && ShopSubsystem->CanBuyItem(ItemID, PlayerInventory);
}

FName AFTHubShop::GetCurrencyItemID() const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem ? ShopSubsystem->GetCurrencyItemID() : NAME_None;
}

int32 AFTHubShop::GetCurrencyAmount(UFTInventoryComponent* PlayerInventory) const
{
	const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem();
	return ShopSubsystem ? ShopSubsystem->GetCurrencyAmount(PlayerInventory) : 0;
}

void AFTHubShop::GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const
{
	if (const UFTShopSubsystem* ShopSubsystem = GetShopSubsystem())
	{
		ShopSubsystem->GetShopItems(OutShopItems);
	}
}

UFTShopSubsystem* AFTHubShop::GetShopSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFTShopSubsystem>() : nullptr;
}
