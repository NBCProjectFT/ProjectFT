#include "FTTradePostEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "FTTradePostListObject.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"

void UFTTradePostEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTTradePostListObject* PostObject = Cast<UFTTradePostListObject>(ListItemObject);
	if (!PostObject)
	{
		return;
	}

	const FTTradePostStruct& Post = PostObject->GetTradePost();
	const FName ItemID = Post.GetResolvedItemID();
	const UFTItemDataAsset* ItemDataAsset = nullptr;
	if (!ItemID.IsNone())
	{
		ItemDataAsset = UFTItemFunctionLibrary::FindItemData(this, ItemID);
	}

	const FText ItemName = ItemDataAsset && !ItemDataAsset->ItemData.ItemName.IsEmpty()
		? ItemDataAsset->ItemData.ItemName
		: FText::FromName(ItemID);

	if (TXT_PostTitle)
	{
		TXT_PostTitle->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TXT_PostItem)
	{
		TXT_PostItem->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TXT_PostType)
	{
		TXT_PostType->SetText(Post.bBuyRequest ? FText::FromString(TEXT("삽니다")) : FText::FromString(TEXT("팝니다")));
		TXT_PostType->SetColorAndOpacity(Post.bBuyRequest
			? FSlateColor(FLinearColor(0.15f, 0.85f, 0.35f, 1.0f))
			: FSlateColor(FLinearColor(0.95f, 0.2f, 0.2f, 1.0f)));
	}

	if (TXT_PostItemName)
	{
		const int32 DisplayCount = FMath::Max(1, Post.Count);
		TXT_PostItemName->SetText(DisplayCount > 1
			? FText::FromString(FString::Printf(TEXT("%s x%d"), *ItemName.ToString(), DisplayCount))
			: ItemName);
	}

	if (TXT_PostDescription)
	{
		const FString DescriptionString = Post.Description.ToString().TrimStartAndEnd();
		if (DescriptionString.IsEmpty())
		{
			TXT_PostDescription->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TXT_PostDescription->SetText(FText::FromString(DescriptionString));
			TXT_PostDescription->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (TXT_PostPrice)
	{
		TXT_PostPrice->SetText(FText::FromString(FString::Printf(
			TEXT("%s %d C"),
			Post.bBuyRequest ? TEXT("구매가") : TEXT("판매가"),
			Post.Price)));
	}

	if (IMG_PostItemIcon)
	{
		UTexture2D* IconTexture = ItemDataAsset ? ItemDataAsset->ItemData.ItemIcon.LoadSynchronous() : nullptr;
		if (IconTexture)
		{
			IMG_PostItemIcon->SetBrushFromTexture(IconTexture, true);
			IMG_PostItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_PostItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
