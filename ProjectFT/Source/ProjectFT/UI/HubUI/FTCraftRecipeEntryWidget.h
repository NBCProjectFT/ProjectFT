#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTCraftRecipeEntryWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class PROJECTFT_API UFTCraftRecipeEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RecipeNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_ResultItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RequiredItemsText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ResultItemText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ResultCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_CraftableState;
};
