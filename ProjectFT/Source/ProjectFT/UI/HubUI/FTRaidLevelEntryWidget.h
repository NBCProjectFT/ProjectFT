#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTRaidLevelEntryWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTRaidLevelEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_LevelName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_LevelPreview;

private:
	void ApplySelectionVisual(bool bIsSelected);
};
