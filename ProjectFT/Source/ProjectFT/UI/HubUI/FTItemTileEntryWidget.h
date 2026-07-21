#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FTItemTileEntryWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTItemTileEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
