#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTItemSlotEntryWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UFTItemSlotDataObject;

UCLASS()
class PROJECTFT_API UFTItemSlotEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UBorder> Border_Selected = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_ItemIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SlotIndex = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ItemName = nullptr;

	void ApplyItemSlot(const UFTItemSlotDataObject* ItemSlot);
};
