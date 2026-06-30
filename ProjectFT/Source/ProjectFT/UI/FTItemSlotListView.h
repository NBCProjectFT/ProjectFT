#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "FTItemSlotListView.generated.h"

UCLASS()
class PROJECTFT_API UFTItemSlotListView : public UListView
{
	GENERATED_BODY()

public:
	void SetItemSlotEntryWidgetClass(TSubclassOf<UUserWidget> NewEntryWidgetClass);
};
