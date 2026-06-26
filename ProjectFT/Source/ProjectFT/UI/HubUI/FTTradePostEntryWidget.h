#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTTradePostEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTFT_API UFTTradePostEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_PostTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PostPrice;
};
