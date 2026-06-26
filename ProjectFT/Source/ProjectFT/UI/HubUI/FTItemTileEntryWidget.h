#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTItemTileEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UCheckBox;
class UFTItemTileListObject;

UCLASS()
class PROJECTFT_API UFTItemTileEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemWeight;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* CHK_ItemSelected;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_Locked;

private:
	UFUNCTION()
	void HandleItemCheckStateChanged(bool bIsChecked);

	UPROPERTY()
	TObjectPtr<UFTItemTileListObject> CurrentTileObject;
};
