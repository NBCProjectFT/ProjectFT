#include "FTItemSlotListView.h"

void UFTItemSlotListView::SetItemSlotEntryWidgetClass(TSubclassOf<UUserWidget> NewEntryWidgetClass)
{
	EntryWidgetClass = NewEntryWidgetClass;
}
