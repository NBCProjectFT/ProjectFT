#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTHubStorageViewModel.generated.h"

class AFTHubStorage;
class UFTInventoryComponent;
class UFTStorageSubsystem;

UENUM(BlueprintType)
enum class EFTHubStorageTransferSource : uint8
{
	/** @brief 선택된 이동 출발지가 없음. */
	None,

	/** @brief 플레이어 인벤토리에서 창고로 넣는 방향. */
	Player,

	/** @brief 창고에서 플레이어 인벤토리로 꺼내는 방향. */
	Storage
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTHubStorageViewModelChanged);

/**
 * @brief 창고 UI의 표시 데이터와 버튼 명령을 관리하는 ViewModel.
 *
 * UFTHubStorageWidget은 UMG 위젯 바인딩과 입력 이벤트만 담당하고,
 * 이 클래스는 플레이어/창고 목록 만들기, 필터링, 선택 상태, 아이템 이동 명령을 담당한다.
 *
 * MVVM 관점에서 보면:
 * - Model: UFTInventoryComponent, UFTStorageSubsystem
 * - ViewModel: UFTHubStorageViewModel
 * - View: UFTHubStorageWidget / WBP_FTHubStorageWidget
 *
 * @see UFTHubStorageWidget
 * @see UFTStorageSubsystem
 */
UCLASS(BlueprintType)
class PROJECTFT_API UFTHubStorageViewModel : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 창고 액터와 플레이어 인벤토리를 연결하고 화면 데이터를 새로 만든다.
	 */
	void Initialize(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory);

	/** @brief 플레이어 아이템 TileView/ListView에 넣을 UObject 목록. */
	const TArray<TObjectPtr<UObject>>& GetPlayerItemObjects() const;

	/** @brief 창고 아이템 TileView/ListView에 넣을 UObject 목록. */
	const TArray<TObjectPtr<UObject>>& GetStorageItemObjects() const;

	/** @brief 현재 선택 상태를 UI 텍스트로 반환한다. */
	FText GetSelectedItemText() const;

	/** @brief 플레이어 현재 무게와 최대 무게를 UI 텍스트로 반환한다. */
	FText GetPlayerWeightText() const;
	FText GetMoveQuantityText() const;

	/** @brief 현재 선택된 아이템 엔트리 개수. */
	int32 GetSelectedEntryCount() const;

	/** @brief 플레이어 목록에서 선택된 엔트리 개수. */
	int32 GetPlayerSelectedEntryCount() const;

	/** @brief 창고 목록에서 선택된 엔트리 개수. */
	int32 GetStorageSelectedEntryCount() const;

	/** @brief 선택 보관 버튼을 누를 수 있는지 반환한다. */
	bool CanStoreSelected() const;

	/** @brief 선택 꺼내기 버튼을 누를 수 있는지 반환한다. */
	bool CanTakeSelected() const;
	bool CanDecreaseMoveQuantity() const;
	bool CanIncreaseMoveQuantity() const;
	bool CanSetMoveQuantityToHalf() const;
	bool CanSetMoveQuantityToMax() const;

	/** @brief 모두 보관 버튼을 누를 수 있는지 반환한다. */
	bool CanStoreAll() const;

	/** @brief 모두 꺼내기 버튼을 누를 수 있는지 반환한다. */
	bool CanTakeAll() const;

	/** @brief 플레이어/창고 목록을 다시 만들고 OnChanged를 방송한다. */
	void RefreshAll();

	/**
	 * @brief View가 전달한 선택 UObject 목록을 창고 이동용 데이터로 변환한다.
	 *
	 * @param SourceType 선택이 발생한 목록. Player면 플레이어 목록, Storage면 창고 목록.
	 * @param Items ListView/TileView에서 선택된 UObject 목록.
	 */
	void SetSelectedItems(EFTHubStorageTransferSource SourceType, const TArray<UObject*>& Items);

	/** @brief 현재 선택 상태를 비운다. */
	void ClearSelection();
	void IncreaseMoveQuantity();
	void DecreaseMoveQuantity();
	void SetMoveQuantityToHalf();
	void SetMoveQuantityToMax();

	/** @brief 플레이어 목록의 카테고리 필터를 변경한다. */
	void SetPlayerFilter(EFTItemCategoryType FilterCategory);

	/** @brief 창고 목록의 카테고리 필터를 변경한다. */
	void SetStorageFilter(EFTItemCategoryType FilterCategory);

	/** @brief 선택된 플레이어 아이템을 창고로 보낸다. */
	bool StoreSelectedItems();

	/** @brief 선택된 창고 아이템을 플레이어에게 보낸다. */
	bool TakeSelectedItems();

	/** @brief 플레이어가 가진 모든 아이템을 창고로 보낸다. */
	bool StoreAllItems();

	/** @brief 창고가 가진 모든 아이템을 플레이어에게 보낸다. */
	bool TakeAllItems();

	/**
	 * @brief ViewModel 데이터가 바뀌었음을 View에 알리는 이벤트.
	 *
	 * View는 이 이벤트를 받으면 TileView 목록과 버튼 활성화 상태를 다시 그린다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Hub|Storage")
	FFTHubStorageViewModelChanged OnChanged;

private:
	/** @brief 플레이어 또는 창고 인벤토리가 바뀌면 UI 데이터 전체를 다시 만든다. */
	UFUNCTION()
	void HandleInventoryChanged();

	/** @brief 플레이어 인벤토리를 현재 필터에 맞는 UI 항목 목록으로 변환한다. */
	void RefreshPlayerItems();

	/** @brief 창고 인벤토리를 현재 필터에 맞는 UI 항목 목록으로 변환한다. */
	void RefreshStorageItems();

	/** @brief 플레이어/창고 인벤토리 변경 이벤트를 구독한다. */
	void BindInventoryDelegates();

	/** @brief 이전 인벤토리 변경 이벤트 구독을 해제한다. */
	void UnbindInventoryDelegates();

	/** @brief GameInstance에서 창고 서브시스템을 가져온다. */
	UFTStorageSubsystem* GetStorageSubsystem() const;

	/** @brief HubStorage 액터가 보유한 창고 인벤토리를 가져온다. */
	UFTInventoryComponent* GetStorageInventory() const;

	/** @brief 현재 창고 아이템 목록을 서브시스템을 통해 가져온다. */
	void GetCurrentStorageItems(TArray<FTStorageItemStruct>& OutItems) const;

	/** @brief 아이템이 현재 카테고리 필터에 표시되어야 하는지 검사한다. */
	bool ShouldShowItem(FName ItemID, EFTItemCategoryType FilterCategory) const;

	/** @brief 아이템 데이터 에셋을 찾아 카테고리를 읽는다. */
	EFTItemCategoryType GetItemCategory(FName ItemID) const;

	/** @brief TileView/ListView 아이템 UObject를 창고 이동용 구조체로 변환한다. */
	bool TryReadItemObject(UObject* ItemObject, FTStorageItemStruct& OutItem) const;

	/** @brief ViewModel 선택 상태를 양쪽 타일 체크 상태에 반영한다. */
	void SyncSelectionChecks();
	int32 GetMaxMoveQuantity() const;
	void ClampMoveQuantity();

	/** @brief 선택된 아이템들을 SourceType 방향에 맞춰 이동한다. */
	bool TransferSelectedItems(EFTHubStorageTransferSource SourceType);

	/** @brief SourceType 쪽의 모든 아이템을 반대편으로 이동한다. */
	bool TransferAllItems(EFTHubStorageTransferSource SourceType);

	/** @brief 현재 연결된 월드 창고 액터. */
	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage;

	/** @brief 현재 창고 UI를 연 플레이어의 인벤토리. */
	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	/** @brief 플레이어 목록에 표시할 UI 객체들. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PlayerItemObjects;

	/** @brief 창고 목록에 표시할 UI 객체들. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> StorageItemObjects;

	/** @brief 현재 선택된 아이템 ID/수량 목록. */
	TArray<FTStorageItemStruct> SelectedItems;
	int32 MoveQuantity = 1;

	/** @brief 현재 선택이 어느 목록에서 발생했는지. */
	EFTHubStorageTransferSource SelectedSource = EFTHubStorageTransferSource::None;

	/** @brief 플레이어 목록 필터. None이면 전체 표시. */
	EFTItemCategoryType PlayerFilterCategory = EFTItemCategoryType::None;

	/** @brief 창고 목록 필터. None이면 전체 표시. */
	EFTItemCategoryType StorageFilterCategory = EFTItemCategoryType::None;
};
