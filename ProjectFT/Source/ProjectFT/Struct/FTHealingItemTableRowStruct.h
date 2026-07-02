#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTItemDataStruct.h"
#include "FTHealingItemTableRowStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FTHealingItemTableRowStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing Item")
	FTItemDataStruct ItemData;
};
