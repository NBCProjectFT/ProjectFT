#include "FTHubStorageWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "FTStorageItemListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubStorage.h"

void UFTHubStorageWidget::InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory)
{
	HubStorage = InHubStorage;
	PlayerInventory = InPlayerInventory;
	SelectedItemID = NAME_None;
	SelectedSource = EStorageTransferSourceType::None;
	RefreshAllItems();
}

void UFTHubStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_PlayerItems)
	{
		LV_PlayerItems->OnItemClicked().RemoveAll(this);
		LV_PlayerItems->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandlePlayerItemClicked);
	}

	if (LV_StorageItems)
	{
		LV_StorageItems->OnItemClicked().RemoveAll(this);
		LV_StorageItems->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandleStorageItemClicked);
	}

	if (BTN_Store)
	{
		BTN_Store->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStoreClicked);
		BTN_Store->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStoreClicked);
	}

	if (BTN_Take)
	{
		BTN_Take->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleTakeClicked);
		BTN_Take->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleTakeClicked);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleCloseClicked);
	}

	RefreshAllItems();
}

void UFTHubStorageWidget::RefreshAllItems()
{
	RefreshPlayerItems();
	RefreshStorageItems();
	UpdateTransferControls();
}

void UFTHubStorageWidget::RefreshPlayerItems()
{
	if (!LV_PlayerItems)
	{
		return;
	}

	LV_PlayerItems->ClearListItems();

	if (!PlayerInventory)
	{
		return;
	}

	for (const FFTInventoryItem& InventoryItem : PlayerInventory->GetItems())
	{
		UFTStorageItemListObject* ItemObject = NewObject<UFTStorageItemListObject>(this);
		const FTStorageItemStruct StorageItem = { InventoryItem.ItemId, InventoryItem.Quantity };
		ItemObject->Initialize(StorageItem);
		LV_PlayerItems->AddItem(ItemObject);
	}
}

void UFTHubStorageWidget::RefreshStorageItems()
{
	if (!LV_StorageItems)
	{
		return;
	}

	LV_StorageItems->ClearListItems();

	if (!HubStorage)
	{
		return;
	}

	for (const FTStorageItemStruct& StorageItem : HubStorage->GetStorageItems())
	{
		UFTStorageItemListObject* ItemObject = NewObject<UFTStorageItemListObject>(this);
		ItemObject->Initialize(StorageItem);
		LV_StorageItems->AddItem(ItemObject);
	}
}

void UFTHubStorageWidget::UpdateTransferControls()
{
	const int32 SelectedCount = GetSelectedItemCount();
	const bool bHasSelection = !SelectedItemID.IsNone() && SelectedCount > 0;

	if (TXT_SelectedItem)
	{
		TXT_SelectedItem->SetText(bHasSelection
			? FText::FromString(FString::Printf(TEXT("%s x%d"), *SelectedItemID.ToString(), SelectedCount))
			: FText::FromString(TEXT("Select Item")));
	}

	if (SPB_MoveCount)
	{
		SPB_MoveCount->SetMinValue(1.0f);
		SPB_MoveCount->SetMaxValue(FMath::Max(1, SelectedCount));
		SPB_MoveCount->SetValue(bHasSelection ? 1.0f : 0.0f);
		SPB_MoveCount->SetIsEnabled(bHasSelection);
	}

	if (BTN_Store)
	{
		BTN_Store->SetIsEnabled(bHasSelection && SelectedSource == EStorageTransferSourceType::Player);
	}

	if (BTN_Take)
	{
		BTN_Take->SetIsEnabled(bHasSelection && SelectedSource == EStorageTransferSourceType::Storage);
	}
}

int32 UFTHubStorageWidget::GetRequestedCount() const
{
	if (!SPB_MoveCount)
	{
		return 1;
	}

	return FMath::Max(1, FMath::RoundToInt(SPB_MoveCount->GetValue()));
}

int32 UFTHubStorageWidget::GetSelectedItemCount() const
{
	if (SelectedItemID.IsNone())
	{
		return 0;
	}

	if (SelectedSource == EStorageTransferSourceType::Player)
	{
		return PlayerInventory
			? PlayerInventory->GetItemQuantity(SelectedItemID)
			: 0;
	}

	if (SelectedSource == EStorageTransferSourceType::Storage)
	{
		return HubStorage
			? HubStorage->GetStorageItemCount(SelectedItemID)
			: 0;
	}

	return 0;
}

void UFTHubStorageWidget::HandlePlayerItemClicked(UObject* Item)
{
	const UFTStorageItemListObject* ItemObject = Cast<UFTStorageItemListObject>(Item);
	if (!ItemObject)
	{
		return;
	}

	SelectedItemID = ItemObject->GetStorageItem().ItemID;
	SelectedSource = EStorageTransferSourceType::Player;
	UpdateTransferControls();
}

void UFTHubStorageWidget::HandleStorageItemClicked(UObject* Item)
{
	const UFTStorageItemListObject* ItemObject = Cast<UFTStorageItemListObject>(Item);
	if (!ItemObject)
	{
		return;
	}

	SelectedItemID = ItemObject->GetStorageItem().ItemID;
	SelectedSource = EStorageTransferSourceType::Storage;
	UpdateTransferControls();
}

void UFTHubStorageWidget::HandleCloseClicked()
{
	if (HubStorage)
	{
		HubStorage->CloseStorageWidget();
	}
}

void UFTHubStorageWidget::HandleStoreClicked()
{
	if (!HubStorage || !PlayerInventory || SelectedSource != EStorageTransferSourceType::Player)
	{
		return;
	}

	if (HubStorage->StoreItemFromInventory(PlayerInventory, SelectedItemID, GetRequestedCount()))
	{
		SelectedItemID = NAME_None;
		SelectedSource = EStorageTransferSourceType::None;
		RefreshAllItems();
	}
}

void UFTHubStorageWidget::HandleTakeClicked()
{
	if (!HubStorage || !PlayerInventory || SelectedSource != EStorageTransferSourceType::Storage)
	{
		return;
	}

	if (HubStorage->TakeItemToInventory(PlayerInventory, SelectedItemID, GetRequestedCount()))
	{
		SelectedItemID = NAME_None;
		SelectedSource = EStorageTransferSourceType::None;
		RefreshAllItems();
	}
}
