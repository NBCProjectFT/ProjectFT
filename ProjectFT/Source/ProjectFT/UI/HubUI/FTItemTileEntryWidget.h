#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTItemTileEntryWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTItemTileEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_ItemIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ItemName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ItemCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_Locked;
};
