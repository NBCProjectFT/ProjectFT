#include "FTMarketViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"
#include "ProjectFT/UI/HubUI/FTTradePostListObject.h"

void UFTMarketViewModel::Initialize(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegate();

	ShopSubsystem = InShopSubsystem;
	PlayerInventory = InPlayerInventory;
	ClearSelection();

	BindInventoryDelegate();
	RefreshAll();
}

const TArray<TObjectPtr<UObject>>& UFTMarketViewModel::GetTradePostObjects() const
{
	return TradePostObjects;
}

const TArray<TObjectPtr<UObject>>& UFTMarketViewModel::GetSelectedPostItemObjects() const
{
	return SelectedPostItemObjects;
}

UFTTradePostListObject* UFTMarketViewModel::GetSelectedPostObject() const
{
	return SelectedPostObject;
}

FText UFTMarketViewModel::GetSelectedPostTitleText() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	return Post ? Post->Title : FText::FromString(TEXT("Select Post"));
}

FText UFTMarketViewModel::GetSelectedPostDescriptionText() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	return Post ? Post->Description : FText::GetEmpty();
}

FText UFTMarketViewModel::GetSelectedPostItemText() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	return Post
		? FText::FromString(FString::Printf(TEXT("%s x%d"), *Post->GetResolvedItemID().ToString(), Post->Count))
		: FText::GetEmpty();
}

FText UFTMarketViewModel::GetSelectedPostPriceText() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	return Post
		? FText::FromString(FString::Printf(TEXT("Price: %d"), Post->Price))
		: FText::GetEmpty();
}

bool UFTMarketViewModel::CanTradeSelectedPost() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	if (!Post || !ShopSubsystem || !PlayerInventory)
	{
		return false;
	}

	return bBuyRequestMode
		? ShopSubsystem->CanSellMarketItem(Post->PostID, PlayerInventory)
		: ShopSubsystem->CanBuyMarketItem(Post->PostID, PlayerInventory);
}

bool UFTMarketViewModel::IsBuyRequestMode() const
{
	return bBuyRequestMode;
}

void UFTMarketViewModel::RefreshAll()
{
	const FName PreviousPostID = SelectedPostObject ? SelectedPostObject->GetTradePost().PostID : NAME_None;

	RefreshTradePosts();
	RestoreSelection(PreviousPostID);
	RefreshSelectedPostItems();
	NotifyChanged();
}

void UFTMarketViewModel::SetBuyRequestMode(const bool bInBuyRequestMode)
{
	if (bBuyRequestMode == bInBuyRequestMode)
	{
		return;
	}

	bBuyRequestMode = bInBuyRequestMode;
	ClearSelection();
	RefreshAll();
}

void UFTMarketViewModel::SelectTradePostObject(UObject* ItemObject)
{
	SelectedPostObject = Cast<UFTTradePostListObject>(ItemObject);
	RefreshSelectedPostItems();
	NotifyChanged();
}

bool UFTMarketViewModel::TradeSelectedPost()
{
	const FTTradePostStruct* Post = GetSelectedPost();
	if (!Post || !ShopSubsystem)
	{
		return false;
	}

	const bool bSuccess = bBuyRequestMode
		? ShopSubsystem->SellMarketItem(Post->PostID, PlayerInventory)
		: ShopSubsystem->BuyMarketItem(Post->PostID, PlayerInventory);

	if (!bSuccess)
	{
		return false;
	}

	RefreshAll();
	return true;
}

void UFTMarketViewModel::HandleInventoryChanged()
{
	RefreshAll();
}

void UFTMarketViewModel::RefreshTradePosts()
{
	TradePostObjects.Reset();

	if (!ShopSubsystem)
	{
		return;
	}

	TArray<FTTradePostStruct> Posts;
	if (bBuyRequestMode)
	{
		ShopSubsystem->GetMarketBuyPosts(Posts);
	}
	else
	{
		ShopSubsystem->GetMarketSellPosts(Posts);
	}

	for (const FTTradePostStruct& Post : Posts)
	{
		UFTTradePostListObject* PostObject = NewObject<UFTTradePostListObject>(this);
		PostObject->Initialize(Post);
		TradePostObjects.Add(PostObject);
	}
}

void UFTMarketViewModel::RefreshSelectedPostItems()
{
	SelectedPostItemObjects.Reset();

	const FTTradePostStruct* Post = GetSelectedPost();
	if (!Post || Post->GetResolvedItemID().IsNone() || Post->Count <= 0)
	{
		return;
	}

	UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
	ItemObject->InitializeItem(Post->GetResolvedItemID(), Post->Count, Post->Price);
	SelectedPostItemObjects.Add(ItemObject);
}

void UFTMarketViewModel::RestoreSelection(const FName PreviousPostID)
{
	SelectedPostObject = nullptr;

	if (PreviousPostID.IsNone())
	{
		return;
	}

	for (UObject* ItemObject : TradePostObjects)
	{
		UFTTradePostListObject* PostObject = Cast<UFTTradePostListObject>(ItemObject);
		if (PostObject && PostObject->GetTradePost().PostID == PreviousPostID)
		{
			SelectedPostObject = PostObject;
			return;
		}
	}
}

void UFTMarketViewModel::ClearSelection()
{
	SelectedPostObject = nullptr;
	SelectedPostItemObjects.Reset();
}

void UFTMarketViewModel::BindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTMarketViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTMarketViewModel::HandleInventoryChanged);
	}
}

void UFTMarketViewModel::UnbindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTMarketViewModel::HandleInventoryChanged);
	}
}

const FTTradePostStruct* UFTMarketViewModel::GetSelectedPost() const
{
	return SelectedPostObject ? &SelectedPostObject->GetTradePost() : nullptr;
}

void UFTMarketViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
