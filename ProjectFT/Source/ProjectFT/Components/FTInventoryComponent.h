#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "FTInventoryComponent.generated.h"

/** @brief 인벤토리 슬롯 한 칸의 정보를 담는 구조체 */
USTRUCT(BlueprintType)
struct FFTInventoryItem
{
	GENERATED_BODY()

public:
	/** @brief 아이템 고유 식별자 (DataAsset의 ItemId와 일치) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemId = NAME_None;

	/** @brief 누적된 아이템 수량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	/** @brief 아이템의 세부 메타데이터 (이름, 아이콘, 무게 등) 정보 포인터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UFTItemDataAsset> ItemDataAsset = nullptr;
};

/** @brief 인벤토리 상태 변경 시 UI 알림용 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnInventoryChanged);

/** @brief 플레이어 캐릭터 등에 부착되는 핵심 인벤토리 컴포넌트 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTInventoryComponent();

	/**
	 * @brief 인벤토리에 특정 아이템을 수량만큼 추가합니다.
	 * @param ItemId : 추가할 아이템의 ID
	 * @param Quantity : 추가할 아이템의 수량
	 * @return 아이템 추가 성공 여부 (무게 초과 시 false)
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	bool AddItem(FName ItemId, int32 Quantity);

	/**
	 * @brief 인벤토리에서 특정 아이템을 수량만큼 제거합니다.
	 * @param ItemId : 제거할 아이템의 ID
	 * @param Quantity : 제거할 아이템의 수량
	 * @return 아이템 제거 성공 여부 (소지 수량 부족 시 false)
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	bool RemoveItem(FName ItemId, int32 Quantity);

	/** @brief 인벤토리의 모든 아이템을 비우고 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void ClearInventory();

	/**
	 * @brief 인벤토리에 들어 있는 실제 아이템 리스트를 반환합니다.
	 * @return 인벤토리 슬롯 배열의 const 참조
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	const TArray<FFTInventoryItem>& GetItems() const { return Items; }

	/**
	 * @brief 인벤토리의 특정 슬롯 인덱스에 보관된 아이템 정보를 조회합니다.
	 * @param SlotIndex : 조회할 인벤토리 슬롯 인덱스
	 * @param OutItem : 반환받을 아이템 정보
	 * @return 해당 슬롯 인덱스가 유효하고 아이템이 존재하면 true
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	bool GetInventoryItemAtIndex(int32 SlotIndex, FFTInventoryItem& OutItem) const;

	/** @brief 상단 카테고리 탭 선택 시 해당하는 타입의 아이템만 필터링하여 반환 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	TArray<FFTInventoryItem> GetItemsByCategory(EFTItemCategoryType Category) const;

	/** @brief 다중 선택된 인덱스들의 아이템 일괄 제거 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	bool RemoveItemsByIndices(const TArray<int32>& TargetIndices);

	/** @brief 인벤토리 아이템을 실제 필드에 드롭해 달라는 런타임 메시지 발행 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void RequestDropItems(const TArray<int32>& TargetIndices);
	
	/**
	 * @brief 현재 인벤토리에 보관된 아이템들의 총 무게를 반환합니다.
	 * @return 현재 총 무게
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	float GetCurrentWeight() const { return CurrentWeight; }

	/**
	 * @brief 인벤토리가 소지할 수 있는 최대 소지 가능 무게를 반환합니다.
	 * @return 최대 소지 가능 무게
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	float GetMaxWeight() const { return MaxWeight; }

	/**
	 * @brief 최대 소지 가능 무게를 변경합니다.
	 * @param NewMaxWeight : 새로 설정할 최대 소지 가능 무게
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetMaxWeight(float NewMaxWeight);
	
	/**
	 * @brief 특정 아이템의 현재 누적된 총 수량을 반환합니다.
	 * @param ItemId : 조회할 아이템의 ID
	 * @return 해당 아이템의 누적 수량 (없을 경우 0)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory|Item")
	const int32 GetItemQuantity(FName ItemId) const;
	
	/** @brief 인벤토리 상태 변경 시 호출되는 이벤트 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "FT|Inventory")
	FFTOnInventoryChanged OnInventoryChanged;

	/**
	 * @brief 특정 아이템의 UFTItemDataAsset 메모리 주소를 반환합니다.
	 * @param ItemId : 조회할 아이템의 ID
	 * @return UFTItemDataAsset의 메모리 주소(없을 경우 nullptr)
	 */
	const UFTItemDataAsset* GetItemPtr(FName ItemId) const;

	/**
	 * @brief ItemId를 통해 메타데이터 에셋을 검색하는 전역 헬퍼 함수
	 * @param ItemId : 검색할 아이템의 ID
	 * @return 에셋 매니저에서 로드된 UFTItemDataAsset의 포인터
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	UFTItemDataAsset* FindItemData(FName ItemId) const;

	/**
	 * @brief 특정 퀵슬롯에 아이템을 지정합니다. 일반(Common) 아이템은 등록이 제한됩니다.
	 * @param SlotIndex : 대상 퀵슬롯 인덱스 (0 ~ 5)
	 * @param ItemId : 등록할 아이템 ID
	 * @return 등록 성공 여부 (일반 아이템이거나 인덱스가 벗어나면 false)
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory|QuickSlot")
	bool SetQuickSlot(int32 SlotIndex, FName ItemId);

	/**
	 * @brief N번 퀵슬롯에 지정된 아이템 상세 정보를 조회합니다.
	 * @param SlotIndex : 조회할 퀵슬롯 인덱스 (0 ~ 5)
	 * @param OutItem : 반환받을 아이템 정보 구조체
	 * @return 슬롯에 아이템이 등록되어 있으면 true (인벤토리에 없을 경우 수량은 0)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory|QuickSlot")
	bool GetQuickSlotItem(int32 SlotIndex, FFTInventoryItem& OutItem) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** @brief 인벤토리에 보관된 모든 아이템들의 무게를 합산하여 실시간 갱신합니다. */
	void UpdateWeight();

protected:
	/**
	 * @brief GameplayMessageSubsystem을 통해 아이템 획득 메시지를 수신했을 때 호출됩니다.
	 * @param Channel : 메시지 채널 태그
	 * @param Payload : 아이템 획득 메시지 페이로드 데이터
	 */
	void HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	
	/**
	 * @brief GameplayMessageSubsystem을 통해 아이템 사용 메시지를 수신했을 때 호출됩니다.
	 * @param Channel : 메시지 채널 태그
	 * @param Payload : 아이템 사용 메시지 페이로드 데이터
	 */
	void HandleItemConsumedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	
protected:
	/** @brief 인벤토리에 들어 있는 실제 아이템 리스트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Inventory")
	TArray<FFTInventoryItem> Items;

	/** @brief 퀵슬롯 6칸에 저장될 아이템 ID 목록 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Inventory|QuickSlot")
	TArray<FName> QuickSlots;

	/** @brief 최대 소지 가능 무게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Inventory|Capacity")
	float MaxWeight = 100.0f;

	/** @brief 현재 총 무게 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Inventory|Capacity")
	float CurrentWeight = 0.0f;

private:
	/** @brief 아이템 획득 메시지 구독 해제용 핸들 */
	FGameplayMessageListenerHandle PickedUpListenerHandle;

	/** @brief 아이템 사용 메시지 구독 해제용 핸들 */
	FGameplayMessageListenerHandle ConsumedListenerHandle;
};
