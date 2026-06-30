#include "FTItemSlotEntryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "../ViewModel/FTHUDViewModel.h"

void UFTItemSlotEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	ApplyItemSlot(Cast<UFTItemSlotDataObject>(ListItemObject));
}

void UFTItemSlotEntryWidget::ApplyItemSlot(const UFTItemSlotDataObject* ItemSlot)
{
	if (!ItemSlot)
	{
		return;
	}

	if (Border_Selected)
	{
		Border_Selected->SetBrushColor(ItemSlot->bSelected ? FLinearColor(1.0f, 0.72f, 0.08f, 0.85f) : FLinearColor(0.03f, 0.03f, 0.03f, 0.65f));
	}

	if (IMG_ItemIcon)
	{
		IMG_ItemIcon->SetBrushFromTexture(ItemSlot->Icon);
		IMG_ItemIcon->SetVisibility(ItemSlot->Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Text_SlotIndex)
	{
		Text_SlotIndex->SetText(FText::AsNumber(ItemSlot->SlotIndex + 1));
	}

	if (Text_ItemName)
	{
		Text_ItemName->SetText(ItemSlot->DisplayName);
	}
}
