#include "FTShopItemEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTShopItemListObject.h"

void UFTShopItemEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTShopItemListObject* ShopItemObject = Cast<UFTShopItemListObject>(ListItemObject);
	if (!ShopItemObject || !ItemNameText)
	{
		return;
	}

	const FTShopItemStruct& ShopItem = ShopItemObject->GetShopItem();
	const bool bUnlocked = ShopItemObject->IsUnlocked();
	const FSlateColor TextColor = bUnlocked
		? FSlateColor(FLinearColor::White)
		: FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f));

	ItemNameText->SetText(FText::FromName(ShopItem.ItemID));
	ItemNameText->SetColorAndOpacity(TextColor);

	if (ItemCountText)
	{
		ItemCountText->SetText(FText::AsNumber(ShopItem.Count));
		ItemCountText->SetColorAndOpacity(TextColor);
	}

	if (ItemPriceText)
	{
		ItemPriceText->SetText(FText::FromString(FString::Printf(TEXT("%d"), ShopItem.Price)));
		ItemPriceText->SetColorAndOpacity(TextColor);
	}

	if (ItemStateText)
	{
		ItemStateText->SetText(bUnlocked
			? FText::FromString(TEXT("구매 가능"))
			: FText::FromString(TEXT("잠김")));
		ItemStateText->SetColorAndOpacity(TextColor);
	}
}
