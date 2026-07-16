#include "FTHubStorageWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "InputCoreTypes.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"
#include "ProjectFT/ViewModel/FTHubStorageViewModel.h"
#include "Types/SlateEnums.h"

void UFTHubStorageWidget::InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory, UFTHubStorageViewModel* InViewModel)
{
	HubStorage = InHubStorage;

	// UIManager가 ViewModel을 재사용할 수 있으므로, 다른 ViewModel이 들어온 경우에만 이벤트 연결을 갈아낀다.
	if (ViewModel != InViewModel)
	{
		if (ViewModel)
		{
			ViewModel->OnChanged.RemoveDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
		}

		ViewModel = InViewModel ? InViewModel : NewObject<UFTHubStorageViewModel>(this);
		if (ViewModel)
		{
			ViewModel->OnChanged.RemoveDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
			ViewModel->OnChanged.AddDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
		}
	}

	if (ViewModel)
	{
		ViewModel->Initialize(HubStorage, InPlayerInventory);
	}

	RefreshFromViewModel();
}

void UFTHubStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	// TileView는 UListView를 상속하므로 선택 이벤트는 ListView API로 처리한다.
	if (UListView* PlayerItemsView = GetPlayerItemsView())
	{
		PlayerItemsView->SetSelectionMode(ESelectionMode::Multi);
		PlayerItemsView->OnItemClicked().RemoveAll(this);
		PlayerItemsView->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandlePlayerItemClicked);
		PlayerItemsView->OnItemSelectionChanged().RemoveAll(this);
	}

	if (UListView* StorageItemsView = GetStorageItemsView())
	{
		StorageItemsView->SetSelectionMode(ESelectionMode::Multi);
		StorageItemsView->OnItemClicked().RemoveAll(this);
		StorageItemsView->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandleStorageItemClicked);
		StorageItemsView->OnItemSelectionChanged().RemoveAll(this);
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

	if (BTN_StoreAll)
	{
		BTN_StoreAll->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStoreAllClicked);
		BTN_StoreAll->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStoreAllClicked);
	}

	if (BTN_TakeAll)
	{
		BTN_TakeAll->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleTakeAllClicked);
		BTN_TakeAll->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleTakeAllClicked);
	}

	if (BTN_QuantityMinus)
	{
		BTN_QuantityMinus->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleQuantityMinusClicked);
		BTN_QuantityMinus->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleQuantityMinusClicked);
	}

	if (BTN_QuantityPlus)
	{
		BTN_QuantityPlus->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleQuantityPlusClicked);
		BTN_QuantityPlus->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleQuantityPlusClicked);
	}

	if (BTN_QuantityHalf)
	{
		BTN_QuantityHalf->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleQuantityHalfClicked);
		BTN_QuantityHalf->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleQuantityHalfClicked);
	}

	if (BTN_QuantityMax)
	{
		BTN_QuantityMax->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleQuantityMaxClicked);
		BTN_QuantityMax->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleQuantityMaxClicked);
	}

	if (BTN_PlayerFilterAll)
	{
		BTN_PlayerFilterAll->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterAllClicked);
		BTN_PlayerFilterAll->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterAllClicked);
	}

	if (BTN_PlayerFilterWeapon)
	{
		BTN_PlayerFilterWeapon->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterWeaponClicked);
		BTN_PlayerFilterWeapon->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterWeaponClicked);
	}

	if (BTN_PlayerFilterHealing)
	{
		BTN_PlayerFilterHealing->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterHealingClicked);
		BTN_PlayerFilterHealing->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterHealingClicked);
	}

	if (BTN_PlayerFilterCommon)
	{
		BTN_PlayerFilterCommon->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterCommonClicked);
		BTN_PlayerFilterCommon->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandlePlayerFilterCommonClicked);
	}

	if (BTN_StorageFilterAll)
	{
		BTN_StorageFilterAll->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStorageFilterAllClicked);
		BTN_StorageFilterAll->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStorageFilterAllClicked);
	}

	if (BTN_StorageFilterWeapon)
	{
		BTN_StorageFilterWeapon->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStorageFilterWeaponClicked);
		BTN_StorageFilterWeapon->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStorageFilterWeaponClicked);
	}

	if (BTN_StorageFilterHealing)
	{
		BTN_StorageFilterHealing->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStorageFilterHealingClicked);
		BTN_StorageFilterHealing->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStorageFilterHealingClicked);
	}

	if (BTN_StorageFilterCommon)
	{
		BTN_StorageFilterCommon->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleStorageFilterCommonClicked);
		BTN_StorageFilterCommon->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleStorageFilterCommonClicked);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubStorageWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubStorageWidget::HandleCloseClicked);
	}

	RefreshFromViewModel();
}

FReply UFTHubStorageWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		HandleCloseClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTHubStorageWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	PlayerSelectedItems.Reset();
	for (UObject* Item : ViewModel->GetPlayerItemObjects())
	{
		if (const UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(Item);
			TileObject && TileObject->IsChecked())
		{
			PlayerSelectedItems.Add(TWeakObjectPtr<UObject>(Item));
		}
	}

	StorageSelectedItems.Reset();
	for (UObject* Item : ViewModel->GetStorageItemObjects())
	{
		if (const UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(Item);
			TileObject && TileObject->IsChecked())
		{
			StorageSelectedItems.Add(TWeakObjectPtr<UObject>(Item));
		}
	}

	// 목록을 다시 채울 때 ListView가 선택 변경 이벤트를 낼 수 있어서, 재진입을 막는다.
	bRefreshingFromViewModel = true;
	PopulateItems(GetPlayerItemsView(), ViewModel->GetPlayerItemObjects());
	PopulateItems(GetStorageItemsView(), ViewModel->GetStorageItemObjects());
	bRefreshingFromViewModel = false;

	if (TXT_SelectedItem)
	{
		TXT_SelectedItem->SetText(ViewModel->GetSelectedItemText());
	}

	if (TXT_PlayerWeight)
	{
		TXT_PlayerWeight->SetText(ViewModel->GetPlayerWeightText());
	}

	if (SPB_MoveCount)
	{
		const int32 SelectedCount = ViewModel->GetSelectedEntryCount();
		const bool bHasSelection = SelectedCount > 0;
		SPB_MoveCount->SetMinValue(1.0f);
		SPB_MoveCount->SetMaxValue(999.0f);
		SPB_MoveCount->SetValue(bHasSelection ? FCString::Atoi(*ViewModel->GetMoveQuantityText().ToString()) : 0.0f);
		SPB_MoveCount->SetIsEnabled(false);
	}

	if (TXT_MoveQuantity)
	{
		TXT_MoveQuantity->SetText(ViewModel->GetMoveQuantityText());
	}

	if (TXT_TradeQuantity)
	{
		TXT_TradeQuantity->SetText(ViewModel->GetMoveQuantityText());
	}

	if (BTN_Store)
	{
		BTN_Store->SetIsEnabled(ViewModel->CanStoreSelected());
	}

	if (BTN_Take)
	{
		BTN_Take->SetIsEnabled(ViewModel->CanTakeSelected());
	}

	if (BTN_StoreAll)
	{
		BTN_StoreAll->SetIsEnabled(ViewModel->CanStoreAll());
	}

	if (BTN_TakeAll)
	{
		BTN_TakeAll->SetIsEnabled(ViewModel->CanTakeAll());
	}

	if (BTN_QuantityMinus)
	{
		BTN_QuantityMinus->SetIsEnabled(ViewModel->CanDecreaseMoveQuantity());
	}

	if (BTN_QuantityPlus)
	{
		BTN_QuantityPlus->SetIsEnabled(ViewModel->CanIncreaseMoveQuantity());
	}

	if (BTN_QuantityHalf)
	{
		BTN_QuantityHalf->SetIsEnabled(ViewModel->CanSetMoveQuantityToHalf());
	}

	if (BTN_QuantityMax)
	{
		BTN_QuantityMax->SetIsEnabled(ViewModel->CanSetMoveQuantityToMax());
	}
}

UListView* UFTHubStorageWidget::GetPlayerItemsView() const
{
	return Cast<UListView>(TV_PlayerItems);
}

UListView* UFTHubStorageWidget::GetStorageItemsView() const
{
	return Cast<UListView>(TV_StorageItems);
}

void UFTHubStorageWidget::PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items)
{
	if (!ItemsView)
	{
		return;
	}

	// ViewModel이 만든 UObject 목록이 TileView 엔트리 위젯의 데이터 소스가 된다.
	ItemsView->ClearListItems();
	for (UObject* Item : Items)
	{
		ItemsView->AddItem(Item);
		if (const UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(Item))
		{
			ItemsView->SetItemSelection(Item, TileObject->IsChecked());
		}
	}
}

void UFTHubStorageWidget::PushSelectedItemsToViewModel(UListView* ItemsView, const bool bFromPlayerItems)
{
	if (bRefreshingFromViewModel || !ViewModel || !ItemsView)
	{
		return;
	}

	if (UListView* OtherItemsView = bFromPlayerItems ? GetStorageItemsView() : GetPlayerItemsView())
	{
		bRefreshingFromViewModel = true;
		OtherItemsView->ClearSelection();
		bRefreshingFromViewModel = false;
	}

	TArray<UObject*> SelectedItems;
	ItemsView->GetSelectedItems(SelectedItems);
	// View는 선택된 UObject만 알고, 어떤 아이템/수량인지는 ViewModel이 해석한다.
	ViewModel->SetSelectedItems(
		bFromPlayerItems ? EFTHubStorageTransferSource::Player : EFTHubStorageTransferSource::Storage,
		SelectedItems
	);
}

void UFTHubStorageWidget::HandlePlayerItemClicked(UObject* Item)
{
	if (UListView* PlayerItemsView = GetPlayerItemsView())
	{
		if (Item)
		{
			for (const TWeakObjectPtr<UObject>& SelectedItem : PlayerSelectedItems)
			{
				if (SelectedItem.IsValid() && SelectedItem.Get() != Item)
				{
					PlayerItemsView->SetItemSelection(SelectedItem.Get(), true);
				}
			}

			const bool bWasSelected = PlayerSelectedItems.Contains(TWeakObjectPtr<UObject>(Item));
			PlayerItemsView->SetItemSelection(Item, !bWasSelected);
		}
	}

	PushSelectedItemsToViewModel(GetPlayerItemsView(), true);
}

void UFTHubStorageWidget::HandleStorageItemClicked(UObject* Item)
{
	if (UListView* StorageItemsView = GetStorageItemsView())
	{
		if (Item)
		{
			for (const TWeakObjectPtr<UObject>& SelectedItem : StorageSelectedItems)
			{
				if (SelectedItem.IsValid() && SelectedItem.Get() != Item)
				{
					StorageItemsView->SetItemSelection(SelectedItem.Get(), true);
				}
			}

			const bool bWasSelected = StorageSelectedItems.Contains(TWeakObjectPtr<UObject>(Item));
			StorageItemsView->SetItemSelection(Item, !bWasSelected);
		}
	}

	PushSelectedItemsToViewModel(GetStorageItemsView(), false);
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
	if (ViewModel)
	{
		ViewModel->StoreSelectedItems();
	}
}

void UFTHubStorageWidget::HandleTakeClicked()
{
	if (ViewModel)
	{
		ViewModel->TakeSelectedItems();
	}
}

void UFTHubStorageWidget::HandleStoreAllClicked()
{
	if (ViewModel)
	{
		ViewModel->StoreAllItems();
	}
}

void UFTHubStorageWidget::HandleTakeAllClicked()
{
	if (ViewModel)
	{
		ViewModel->TakeAllItems();
	}
}

void UFTHubStorageWidget::HandleQuantityMinusClicked()
{
	if (ViewModel)
	{
		ViewModel->DecreaseMoveQuantity();
	}
}

void UFTHubStorageWidget::HandleQuantityPlusClicked()
{
	if (ViewModel)
	{
		ViewModel->IncreaseMoveQuantity();
	}
}

void UFTHubStorageWidget::HandleQuantityHalfClicked()
{
	if (ViewModel)
	{
		ViewModel->SetMoveQuantityToHalf();
	}
}

void UFTHubStorageWidget::HandleQuantityMaxClicked()
{
	if (ViewModel)
	{
		ViewModel->SetMoveQuantityToMax();
	}
}

void UFTHubStorageWidget::HandlePlayerFilterAllClicked()
{
	if (ViewModel)
	{
		ViewModel->SetPlayerFilter(EFTItemCategoryType::None);
	}
}

void UFTHubStorageWidget::HandlePlayerFilterWeaponClicked()
{
	if (ViewModel)
	{
		ViewModel->SetPlayerFilter(EFTItemCategoryType::Weapon);
	}
}

void UFTHubStorageWidget::HandlePlayerFilterHealingClicked()
{
	if (ViewModel)
	{
		ViewModel->SetPlayerFilter(EFTItemCategoryType::Healing);
	}
}

void UFTHubStorageWidget::HandlePlayerFilterCommonClicked()
{
	if (ViewModel)
	{
		ViewModel->SetPlayerFilter(EFTItemCategoryType::Common);
	}
}

void UFTHubStorageWidget::HandleStorageFilterAllClicked()
{
	if (ViewModel)
	{
		ViewModel->SetStorageFilter(EFTItemCategoryType::None);
	}
}

void UFTHubStorageWidget::HandleStorageFilterWeaponClicked()
{
	if (ViewModel)
	{
		ViewModel->SetStorageFilter(EFTItemCategoryType::Weapon);
	}
}

void UFTHubStorageWidget::HandleStorageFilterHealingClicked()
{
	if (ViewModel)
	{
		ViewModel->SetStorageFilter(EFTItemCategoryType::Healing);
	}
}

void UFTHubStorageWidget::HandleStorageFilterCommonClicked()
{
	if (ViewModel)
	{
		ViewModel->SetStorageFilter(EFTItemCategoryType::Common);
	}
}
