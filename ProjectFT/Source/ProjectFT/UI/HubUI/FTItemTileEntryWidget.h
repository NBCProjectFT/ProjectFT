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
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* IMG_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemWeight;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* CHK_ItemSelected;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTItemTileListObject> CurrentTileObject;
};
