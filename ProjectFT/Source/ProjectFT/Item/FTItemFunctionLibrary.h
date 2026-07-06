#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FTItemFunctionLibrary.generated.h"

class UFTItemDataAsset;

/**
 * 아이템 시스템 관련 공용 유틸리티 및 데이터 헬퍼 함수를 제공하는 전역 라이브러리
 */
UCLASS()
class PROJECTFT_API UFTItemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief ItemId를 통해 에셋 매니저(Asset Manager)에서 UFTItemDataAsset을 탐색하여 반환합니다.
	 * @param WorldContextObject : 월드 컨텍스트 오브젝트
	 * @param ItemId : 검색할 아이템의 ID
	 * @return 로드된 UFTItemDataAsset 포인터 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Item", meta = (WorldContext = "WorldContextObject"))
	static UFTItemDataAsset* FindItemData(const UObject* WorldContextObject, FName ItemId);

	/**
	 * @brief 로컬 플레이어가 보유한 특정 아이템의 총 수량을 인벤토리 컴포넌트에서 찾아 즉시 반환합니다.
	 * @param WorldContextObject : 월드 컨텍스트 오브젝트
	 * @param ItemId : 조회할 아이템 ID
	 * @return 보유 수량 (없거나 인벤토리를 찾지 못하면 0)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Item", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPlayerItemQuantity(const UObject* WorldContextObject, FName ItemId);

	/**
	 * @brief 대상 액터(예: 플레이어)의 전방 지형 정보를 고려하여 안전한 아이템 드롭 스폰 위치를 계산합니다.
	 * @param InstigatorActor : 드롭을 시작하는 대상 액터
	 * @return 지형 보정이 적용된 월드 스폰 위치
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Item")
	static FVector CalculateDropLocation(AActor* InstigatorActor);
};
