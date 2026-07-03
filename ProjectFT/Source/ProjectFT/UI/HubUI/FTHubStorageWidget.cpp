#include "FTHubStorageWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/ViewModel/FTHubStorageViewModel.h"
#include "Types/SlateEnums.h"

void UFTHubStorageWidget::InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory, UFTHubStorageViewModel* InViewModel)
{
	HubStorage = InHubStorage;

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
		ViewModel->Initialize(HubStorage, InPlayerInventory, TV_PlayerItems != nullptr, TV_StorageItems != nullptr);
	}

	RefreshFromViewModel();
}

void UFTHubStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UListView* PlayerItemsView = GetPlayerItemsView())
	{
		PlayerItemsView->SetSelectionMode(ESelectionMode::Multi);
		PlayerItemsView->OnItemClicked().RemoveAll(this);
		PlayerItemsView->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandlePlayerItemClicked);
		PlayerItemsView->OnItemSelectionChanged().RemoveAll(this);
		PlayerItemsView->OnItemSelectionChanged().AddUObject(this, &UFTHubStorageWidget::HandlePlayerItemSelectionChanged);
	}

	if (UListView* StorageItemsView = GetStorageItemsView())
	{
		StorageItemsView->SetSelectionMode(ESelectionMode::Multi);
		StorageItemsView->OnItemClicked().RemoveAll(this);
		StorageItemsView->OnItemClicked().AddUObject(this, &UFTHubStorageWidget::HandleStorageItemClicked);
		StorageItemsView->OnItemSelectionChanged().RemoveAll(this);
		StorageItemsView->OnItemSelectionChanged().AddUObject(this, &UFTHubStorageWidget::HandleStorageItemSelectionChanged);
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

void UFTHubStorageWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

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
		SPB_MoveCount->SetMaxValue(FMath::Max(1, SelectedCount));
		SPB_MoveCount->SetValue(bHasSelection ? 1.0f : 0.0f);
		SPB_MoveCount->SetIsEnabled(bHasSelection);
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
}

UListView* UFTHubStorageWidget::GetPlayerItemsView() const
{
	return TV_PlayerItems ? Cast<UListView>(TV_PlayerItems) : LV_PlayerItems;
}

UListView* UFTHubStorageWidget::GetStorageItemsView() const
{
	return TV_StorageItems ? Cast<UListView>(TV_StorageItems) : LV_StorageItems;
}

void UFTHubStorageWidget::PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items)
{
	if (!ItemsView)
	{
		return;
	}

	ItemsView->ClearListItems();
	for (UObject* Item : Items)
	{
		ItemsView->AddItem(Item);
	}
}

void UFTHubStorageWidget::PushSelectedItemsToViewModel(UListView* ItemsView, const bool bFromPlayerItems)
{
	if (bRefreshingFromViewModel || !ViewModel || !ItemsView)
	{
		return;
	}

	TArray<UObject*> SelectedItems;
	ItemsView->GetSelectedItems(SelectedItems);
	ViewModel->SetSelectedItems(
		bFromPlayerItems ? EFTHubStorageTransferSource::Player : EFTHubStorageTransferSource::Storage,
		SelectedItems
	);
}

void UFTHubStorageWidget::HandlePlayerItemClicked(UObject* Item)
{
	PushSelectedItemsToViewModel(GetPlayerItemsView(), true);
}

void UFTHubStorageWidget::HandleStorageItemClicked(UObject* Item)
{
	PushSelectedItemsToViewModel(GetStorageItemsView(), false);
}

void UFTHubStorageWidget::HandlePlayerItemSelectionChanged(UObject* Item)
{
	PushSelectedItemsToViewModel(GetPlayerItemsView(), true);
}

void UFTHubStorageWidget::HandleStorageItemSelectionChanged(UObject* Item)
{
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
