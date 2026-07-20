#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTItemDataStruct.h"
#include "FTItemTableRowStruct.generated.h"

/**
 * 구글 시트의 아이템 행 데이터를 받아 DataTable로 변환하기 위한 구조체
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTItemTableRowStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FTItemDataStruct ItemData;
};
