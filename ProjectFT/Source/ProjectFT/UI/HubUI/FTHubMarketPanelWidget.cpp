#include "FTHubMarketPanelWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "FTTradePostListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubShop.h"

void UFTHubMarketPanelWidget::InitializeMarketPanel(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory)
{
	HubShop = InHubShop;
	PlayerInventory = InPlayerInventory;
	SelectedPost = nullptr;
	RefreshTradePosts();
	UpdateSelectedPostDetails();
}

void UFTHubMarketPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_TradePosts)
	{
		LV_TradePosts->OnItemClicked().RemoveAll(this);
		LV_TradePosts->OnItemClicked().AddUObject(this, &UFTHubMarketPanelWidget::HandleTradePostClicked);
	}

	if (BTN_BuyRequestsTab)
	{
		BTN_BuyRequestsTab->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked);
		BTN_BuyRequestsTab->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked);
	}

	if (BTN_SellOffersTab)
	{
		BTN_SellOffersTab->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleSellOffersTabClicked);
		BTN_SellOffersTab->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleSellOffersTabClicked);
	}

	if (BTN_Trade)
	{
		BTN_Trade->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
		BTN_Trade->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
	}

	RefreshTradePosts();
	UpdateSelectedPostDetails();
}

void UFTHubMarketPanelWidget::RefreshTradePosts()
{
	if (!LV_TradePosts)
	{
		return;
	}

	const FName SelectedPostID = SelectedPost
		? SelectedPost->GetTradePost().PostID
		: NAME_None;

	SelectedPost = nullptr;
	LV_TradePosts->ClearListItems();

	if (!HubShop)
	{
		return;
	}

	TArray<FTTradePostStruct> Posts;
	if (bBuyRequestMode)
	{
		HubShop->GetMarketBuyPosts(Posts);
	}
	else
	{
		HubShop->GetMarketSellPosts(Posts);
	}

	for (const FTTradePostStruct& Post : Posts)
	{
		UFTTradePostListObject* PostObject = NewObject<UFTTradePostListObject>(this);
		PostObject->Initialize(Post);
		LV_TradePosts->AddItem(PostObject);

		if (Post.PostID == SelectedPostID)
		{
			SelectedPost = PostObject;
			LV_TradePosts->SetItemSelection(PostObject, true);
		}
	}
}

void UFTHubMarketPanelWidget::UpdateSelectedPostDetails()
{
	const bool bHasSelection = SelectedPost != nullptr;
	const FTTradePostStruct* Post = bHasSelection
		? &SelectedPost->GetTradePost()
		: nullptr;

	if (TXT_SelectedPostTitle)
	{
		TXT_SelectedPostTitle->SetText(Post ? Post->Title : FText::FromString(TEXT("Select Post")));
	}

	if (TXT_SelectedPostDescription)
	{
		TXT_SelectedPostDescription->SetText(Post ? Post->Description : FText::GetEmpty());
	}

	if (TXT_SelectedPostItem)
	{
		TXT_SelectedPostItem->SetText(Post
			? FText::FromString(FString::Printf(TEXT("%s x%d"), *Post->ItemID.ToString(), Post->Count))
			: FText::GetEmpty());
	}

	if (TXT_SelectedPostPrice)
	{
		TXT_SelectedPostPrice->SetText(Post
			? FText::FromString(FString::Printf(TEXT("Price: %d"), Post->Price))
			: FText::GetEmpty());
	}

	if (BTN_Trade)
	{
		BTN_Trade->SetIsEnabled(bHasSelection && PlayerInventory != nullptr);
	}
}

void UFTHubMarketPanelWidget::SetBuyRequestMode(bool bInBuyRequestMode)
{
	bBuyRequestMode = bInBuyRequestMode;
	SelectedPost = nullptr;
	RefreshTradePosts();
	UpdateSelectedPostDetails();
}

void UFTHubMarketPanelWidget::HandleTradePostClicked(UObject* Item)
{
	SelectedPost = Cast<UFTTradePostListObject>(Item);
	UpdateSelectedPostDetails();
}

void UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked()
{
	SetBuyRequestMode(true);
}

void UFTHubMarketPanelWidget::HandleSellOffersTabClicked()
{
	SetBuyRequestMode(false);
}

void UFTHubMarketPanelWidget::HandleTradeClicked()
{
	if (!HubShop || !SelectedPost)
	{
		return;
	}

	const FName PostID = SelectedPost->GetTradePost().PostID;
	const bool bSuccess = bBuyRequestMode
		? HubShop->SellMarketItem(PostID, PlayerInventory)
		: HubShop->BuyMarketItem(PostID, PlayerInventory);

	if (bSuccess)
	{
		RefreshTradePosts();
		UpdateSelectedPostDetails();
	}
}
