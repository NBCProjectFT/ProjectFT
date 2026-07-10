#pragma once

#include "CoreMinimal.h"
#include "FTStorageItemStruct.generated.h"

USTRUCT(BlueprintType)
struct FTStorageItemStruct
{
	GENERATED_BODY()

public:

	/**
	 * @brief 창고에서 다룰 아이템의 데이터 ID.
	 *
	 * 실제 아이템 정의는 FTItemDataAsset 쪽에 있고, 창고는 ID와 수량만 저장한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	/**
	 * @brief 해당 아이템의 개수.
	 *
	 * 초기 창고 목록, UI 목록 표시, 선택된 이동 목록에서 공통으로 사용된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;
};
