#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTCraftRecipeEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTFT_API UFTCraftRecipeEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RecipeNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RequiredItemsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ResultItemText;
};
