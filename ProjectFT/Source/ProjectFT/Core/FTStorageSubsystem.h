#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTStorageSubsystem.generated.h"

class UFTInventoryComponent;

/**
 * @brief 허브 창고의 아이템 이동 규칙을 담당하는 GameInstance 서브시스템.
 *
 * AFTHubStorage 액터는 UI 진입점과 창고 인벤토리 보유만 담당하고,
 * 실제 보관/꺼내기/합산/소모 규칙은 이 클래스에 모아둔다.
 *
 * 현재 구현은 기존 UFTInventoryComponent API를 감싼 얇은 서비스 계층이다.
 * 따라서 인벤토리 컴포넌트 자체의 규칙을 바꾸지 않고 창고 전용 흐름을 확장할 수 있다.
 *
 * @see AFTHubStorage
 * @see UFTInventoryComponent
 */
UCLASS()
class PROJECTFT_API UFTStorageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 창고 인벤토리에 초기 아이템 목록을 채운다.
	 *
	 * @param StorageInventory 초기화할 창고 인벤토리.
	 * @param InitialItems 추가할 아이템 ID/수량 목록.
	 */
	void InitializeStorage(UFTInventoryComponent* StorageInventory, const TArray<FTStorageItemStruct>& InitialItems) const;

	/** @brief 창고 인벤토리에 아이템을 추가한다. */
	bool AddStorageItem(UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;

	/** @brief 창고 인벤토리에서 아이템을 제거한다. */
	bool RemoveStorageItem(UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;

	/** @brief 창고 인벤토리에 들어 있는 특정 아이템 수량을 반환한다. */
	int32 GetStorageItemCount(const UFTInventoryComponent* StorageInventory, FName ItemID) const;

	/** @brief 창고 인벤토리 전체 목록을 FTStorageItemStruct 배열로 변환한다. */
	void GetStorageItems(const UFTInventoryComponent* StorageInventory, TArray<FTStorageItemStruct>& OutItems) const;

	/**
	 * @brief 플레이어 인벤토리에서 창고 인벤토리로 아이템을 옮긴다.
	 *
	 * 추가 성공 후 제거 실패가 발생하면 창고에 추가한 아이템을 되돌린다.
	 */
	bool StoreItemFromInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* SourceInventory, FName ItemID, int32 Count) const;

	/**
	 * @brief 창고 인벤토리에서 플레이어 인벤토리로 아이템을 옮긴다.
	 *
	 * 플레이어 인벤토리에 추가한 뒤 창고 제거가 실패하면 플레이어 추가분을 되돌린다.
	 */
	bool TakeItemToInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* TargetInventory, FName ItemID, int32 Count) const;

	/**
	 * @brief 플레이어 인벤토리와 창고 인벤토리의 특정 아이템 수량을 합산한다.
	 *
	 * 제작/퀘스트처럼 두 보관처를 함께 검사해야 하는 허브 시스템에서 사용한다.
	 */
	int32 GetCombinedItemCount(const UFTInventoryComponent* PlayerInventory, const UFTInventoryComponent* StorageInventory, FName ItemID) const;

	/**
	 * @brief 플레이어 인벤토리를 먼저 소모하고, 부족분을 창고에서 소모한다.
	 *
	 * @return 요청 수량을 모두 소모하면 true.
	 *
	 * @note 현재 구현은 사전 합산 검사 후 순서대로 제거한다. 실패 복구가 필요한 거래성 작업에는
	 * StoreItemFromInventory/TakeItemToInventory처럼 명시적인 되돌림 처리를 추가해야 한다.
	 */
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;

	/**
	 * @brief 디버깅용으로 창고 아이템 목록을 로그에 출력한다.
	 */
	void PrintStorageItems(const UFTInventoryComponent* StorageInventory) const;
};
