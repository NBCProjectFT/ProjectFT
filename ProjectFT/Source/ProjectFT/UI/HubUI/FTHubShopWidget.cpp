#include "FTHubShopWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "FTShopItemListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubShop.h"

void UFTHubShopWidget::InitializeShopWidget(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory)
{
	HubShop = InHubShop;
	PlayerInventory = InPlayerInventory;
	SelectedShopItem = nullptr;
	RefreshShopItems();
	UpdateSelectedItemDetails();
}

void UFTHubShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_ShopItems)
	{
		LV_ShopItems->OnItemClicked().RemoveAll(this);
		LV_ShopItems->OnItemClicked().AddUObject(this, &UFTHubShopWidget::HandleShopItemClicked);
	}

	if (BTN_Buy)
	{
		BTN_Buy->OnClicked.RemoveDynamic(this, &UFTHubShopWidget::HandleBuyClicked);
		BTN_Buy->OnClicked.AddDynamic(this, &UFTHubShopWidget::HandleBuyClicked);
	}

	if (BTN_Refresh)
	{
		BTN_Refresh->OnClicked.RemoveDynamic(this, &UFTHubShopWidget::HandleRefreshClicked);
		BTN_Refresh->OnClicked.AddDynamic(this, &UFTHubShopWidget::HandleRefreshClicked);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubShopWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubShopWidget::HandleCloseClicked);
	}

	RefreshShopItems();
	UpdateSelectedItemDetails();
}

void UFTHubShopWidget::RefreshShopItems()
{
	if (!LV_ShopItems)
	{
		return;
	}

	const FName SelectedItemID = SelectedShopItem
		? SelectedShopItem->GetShopItem().ItemID
		: NAME_None;

	SelectedShopItem = nullptr;
	LV_ShopItems->ClearListItems();

	if (!HubShop)
	{
		return;
	}

	TArray<FTShopItemStruct> ShopItems;
	HubShop->GetShopItems(ShopItems);

	for (const FTShopItemStruct& ShopItem : ShopItems)
	{
		UFTShopItemListObject* ItemObject = NewObject<UFTShopItemListObject>(this);
		ItemObject->Initialize(ShopItem, HubShop->IsShopItemUnlocked(ShopItem.ItemID));
		LV_ShopItems->AddItem(ItemObject);

		if (ShopItem.ItemID == SelectedItemID)
		{
			SelectedShopItem = ItemObject;
			LV_ShopItems->SetItemSelection(ItemObject, true);
		}
	}
}

void UFTHubShopWidget::UpdateSelectedItemDetails()
{
	const bool bHasSelection = SelectedShopItem != nullptr;
	const FTShopItemStruct* ShopItem = bHasSelection
		? &SelectedShopItem->GetShopItem()
		: nullptr;
	const bool bCanBuy = ShopItem && HubShop && HubShop->CanBuyItem(ShopItem->ItemID, PlayerInventory);

	if (TXT_SelectedItemName)
	{
		TXT_SelectedItemName->SetText(ShopItem
			? FText::FromName(ShopItem->ItemID)
			: FText::FromString(TEXT("Select Item")));
	}

	if (TXT_SelectedItemPrice)
	{
		TXT_SelectedItemPrice->SetText(ShopItem
			? FText::FromString(FString::Printf(TEXT("Price: %d"), ShopItem->Price))
			: FText::GetEmpty());
	}

	if (TXT_SelectedItemState)
	{
		TXT_SelectedItemState->SetText(ShopItem
			? (bCanBuy ? FText::FromString(TEXT("구매 가능")) : FText::FromString(TEXT("구매 불가")))
			: FText::GetEmpty());
	}

	if (BTN_Buy)
	{
		BTN_Buy->SetIsEnabled(bCanBuy);
	}
}

void UFTHubShopWidget::HandleShopItemClicked(UObject* Item)
{
	SelectedShopItem = Cast<UFTShopItemListObject>(Item);
	UpdateSelectedItemDetails();
}

void UFTHubShopWidget::HandleBuyClicked()
{
	if (!HubShop || !SelectedShopItem)
	{
		return;
	}

	const FName ItemID = SelectedShopItem->GetShopItem().ItemID;
	if (HubShop->BuyItem(ItemID, PlayerInventory))
	{
		RefreshShopItems();
		UpdateSelectedItemDetails();
	}
}

void UFTHubShopWidget::HandleRefreshClicked()
{
	if (!HubShop)
	{
		return;
	}

	HubShop->RefreshShopItems();
	SelectedShopItem = nullptr;
	RefreshShopItems();
	UpdateSelectedItemDetails();
}

void UFTHubShopWidget::HandleCloseClicked()
{
	if (HubShop)
	{
		HubShop->CloseShopWidget();
	}
}
