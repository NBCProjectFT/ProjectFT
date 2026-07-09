#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTTradePostEntryWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class PROJECTFT_API UFTTradePostEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostType;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_PostItemIcon;
};
