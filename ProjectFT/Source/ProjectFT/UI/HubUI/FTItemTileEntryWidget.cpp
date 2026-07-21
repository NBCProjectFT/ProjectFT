#include "FTItemTileEntryWidget.h"

#include "Components/CheckBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FTItemTileListObject.h"

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

	if (TXT_ItemWeight)
	{
		TXT_ItemWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1fkg"), TileObject->GetUnitWeight())));
	}

	if (CHK_ItemSelected)
	{
		CHK_ItemSelected->OnCheckStateChanged.RemoveAll(this);
		CHK_ItemSelected->SetIsChecked(TileObject->IsChecked());
		CHK_ItemSelected->SetIsEnabled(!bLocked);
		CHK_ItemSelected->SetVisibility(TileObject->ShouldShowSelectionCheckBox()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
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
