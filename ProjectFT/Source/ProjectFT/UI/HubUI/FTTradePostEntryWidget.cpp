#include "FTTradePostEntryWidget.h"

#include "Components/TextBlock.h"
#include "FTTradePostListObject.h"

void UFTTradePostEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTTradePostListObject* PostObject = Cast<UFTTradePostListObject>(ListItemObject);
	if (!PostObject || !TXT_PostTitle)
	{
		return;
	}

	const FTTradePostStruct& Post = PostObject->GetTradePost();
	TXT_PostTitle->SetText(Post.Title);

	if (TXT_PostItem)
	{
		TXT_PostItem->SetText(FText::FromString(FString::Printf(
			TEXT("%s x%d"),
			*Post.GetResolvedItemID().ToString(),
			Post.Count
		)));
	}

	if (TXT_PostPrice)
	{
		TXT_PostPrice->SetText(FText::FromString(FString::Printf(TEXT("%d"), Post.Price)));
	}
}
