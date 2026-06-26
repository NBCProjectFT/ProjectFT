#include "FTItemTileEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FTItemTileListObject.h"

void UFTItemTileEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ListItemObject);
	if (!TileObject || !TXT_ItemName || !TXT_ItemCount)
	{
		return;
	}

	const bool bLocked = TileObject->IsLocked();
	const FSlateColor TextColor = bLocked
		? FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f))
		: FSlateColor(FLinearColor::White);

	TXT_ItemName->SetText(TileObject->GetDisplayName());
	TXT_ItemName->SetColorAndOpacity(TextColor);

	TXT_ItemCount->SetText(FText::FromString(FString::Printf(TEXT("x%d"), TileObject->GetCount())));
	TXT_ItemCount->SetColorAndOpacity(TextColor);

	if (TXT_ItemPrice)
	{
		TXT_ItemPrice->SetText(TileObject->GetPrice() > 0
			? FText::FromString(FString::Printf(TEXT("%d"), TileObject->GetPrice()))
			: FText::GetEmpty());
		TXT_ItemPrice->SetColorAndOpacity(TextColor);
	}

	if (TXT_Locked)
	{
		TXT_Locked->SetText(bLocked
			? FText::FromString(TEXT("잠김"))
			: FText::GetEmpty());
		TXT_Locked->SetColorAndOpacity(TextColor);
	}

	if (IMG_ItemIcon)
	{
		UTexture2D* IconTexture = TileObject->GetItemIcon().LoadSynchronous();
		if (IconTexture)
		{
			IMG_ItemIcon->SetBrushFromTexture(IconTexture);
		}

		IMG_ItemIcon->SetColorAndOpacity(bLocked
			? FLinearColor(0.45f, 0.45f, 0.45f, 1.0f)
			: FLinearColor::White);
	}
}
