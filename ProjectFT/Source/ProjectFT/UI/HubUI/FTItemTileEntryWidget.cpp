#include "FTItemTileEntryWidget.h"

#include "Components/CheckBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FTItemTileListObject.h"
#include "Input/Events.h"

void UFTItemTileEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ListItemObject);
	if (!TileObject)
	{
		return;
	}

	CurrentTileObject = TileObject;

	const bool bLocked = TileObject->IsLocked();

	if (TXT_ItemName)
	{
		TXT_ItemName->SetText(TileObject->GetDisplayName());
	}

	if (TXT_ItemCount)
	{
		TXT_ItemCount->SetText(TileObject->HasOwnedCount()
			? FText::FromString(FString::Printf(TEXT("%d / %d"), TileObject->GetOwnedCount(), TileObject->GetCount()))
			: FText::FromString(FString::Printf(TEXT("x%d"), TileObject->GetCount())));
	}

	if (TXT_ItemOwnedCount)
	{
		TXT_ItemOwnedCount->SetText(TileObject->HasOwnedCount()
			? FText::FromString(FString::Printf(TEXT("%d / %d"), TileObject->GetOwnedCount(), TileObject->GetCount()))
			: FText::GetEmpty());
	}

	if (TXT_ItemWeight)
	{
		TXT_ItemWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1fkg"), TileObject->GetTotalWeight())));
	}

	if (CHK_ItemSelected)
	{
		CHK_ItemSelected->OnCheckStateChanged.RemoveAll(this);
		CHK_ItemSelected->SetIsChecked(TileObject->IsChecked());
		CHK_ItemSelected->SetIsEnabled(!bLocked);
		CHK_ItemSelected->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TXT_ItemPrice)
	{
		TXT_ItemPrice->SetText(TileObject->GetPrice() > 0
			? FText::FromString(FString::Printf(TEXT("%d"), TileObject->GetPrice()))
			: FText::GetEmpty());
	}

	if (TXT_Locked)
	{
		TXT_Locked->SetText(bLocked
			? FText::FromString(TEXT("잠김"))
			: FText::GetEmpty());
	}

	if (IMG_ItemIcon)
	{
		UTexture2D* IconTexture = TileObject->GetItemIcon().LoadSynchronous();
		if (IconTexture)
		{
			IMG_ItemIcon->SetBrushFromTexture(IconTexture);
		}

		IMG_ItemIcon->SetIsEnabled(!bLocked);
	}
}

void UFTItemTileEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	if (CurrentTileObject)
	{
		CurrentTileObject->SetChecked(bIsSelected);
	}

	if (CHK_ItemSelected)
	{
		CHK_ItemSelected->OnCheckStateChanged.RemoveAll(this);
		CHK_ItemSelected->SetIsChecked(bIsSelected);
	}
}

FReply UFTItemTileEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UFTItemTileEntryWidget::HandleItemCheckStateChanged(const bool bIsChecked)
{
}
