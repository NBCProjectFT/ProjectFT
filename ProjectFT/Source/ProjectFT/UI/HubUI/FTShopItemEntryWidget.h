#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTShopItemEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTFT_API UFTShopItemEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ItemCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ItemPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ItemStateText;
};
