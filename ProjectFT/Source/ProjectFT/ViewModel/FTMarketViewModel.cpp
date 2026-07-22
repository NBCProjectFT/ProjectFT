#include "FTMarketViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
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

TArray<UObject*> UFTMarketViewModel::GetTradePostObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(TradePostObjects.Num());
	for (UObject* PostObject : TradePostObjects)
	{
		Result.Add(PostObject);
	}
	return Result;
}

UFTTradePostListObject* UFTMarketViewModel::GetSelectedPostObject() const
{
	return SelectedPostObject;
}

int32 UFTMarketViewModel::GetSelectedItemOwnedCount() const
{
	const FTTradePostStruct* Post = GetSelectedPost();
	if (!Post || !PlayerInventory)
	{
		return 0;
	}

	const FName ItemID = Post->GetResolvedItemID();
	return ItemID.IsNone() ? 0 : PlayerInventory->GetItemQuantity(ItemID);
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
	UFTTradePostListObject* NewSelectedPostObject = Cast<UFTTradePostListObject>(ItemObject);
	if (SelectedPostObject == NewSelectedPostObject)
	{
		return;
	}

	SelectedPostObject = NewSelectedPostObject;
	NotifyChanged();
}

bool UFTMarketViewModel::TradeSelectedPost()
{
	const FTTradePostStruct* Post = GetSelectedPost();
	if (!Post || !ShopSubsystem)
	{
		return false;
	}

	bTransactionInProgress = true;
	const bool bSuccess = bBuyRequestMode
		? ShopSubsystem->SellMarketItem(Post->PostID, PlayerInventory)
		: ShopSubsystem->BuyMarketItem(Post->PostID, PlayerInventory);
	bTransactionInProgress = false;

	if (!bSuccess)
	{
		return false;
	}

	RefreshAll();
	return true;
}

void UFTMarketViewModel::HandleInventoryChanged()
{
	if (!bTransactionInProgress)
	{
		RefreshAll();
	}
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
