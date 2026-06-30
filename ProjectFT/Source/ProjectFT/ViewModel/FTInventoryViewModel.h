#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "FTInventoryViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnViewModelChanged);

/** @brief 인벤토리 UI 연동 및 다중 선택, 무게바 계산 등의 흐름을 관리하는 뷰모델 클래스 (UObject 기반 수동 바인딩 구조) */
UCLASS(BlueprintType)
class PROJECTFT_API UFTInventoryViewModel : public UObject
{
	GENERATED_BODY()

public:
	UFTInventoryViewModel();

	/**
	 * @brief 뷰모델 초기화 및 인벤토리 컴포넌트 이벤트 연동
	 * @param InInventoryComponent : 연동할 인벤토리 컴포넌트
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void Initialize(UFTInventoryComponent* InInventoryComponent);

	/** @brief 현재 필터링 카테고리에 맞는 아이템 ID 목록 (UI 슬롯 목록 바인딩용) */
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	TArray<FName> InventoryItems;

	/** @brief 인벤토리 현재 총 무게 */
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	float CurrentWeight = 0.0f;

	/** @brief 인벤토리 최대 무게 */
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	float MaxWeight = 0.0f;

	/** @brief 우측 상세 정보 패널용 선택된 아이템 ID */
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	FName SelectedItem = NAME_None;

	/** @brief 우측 상세 패널에 표시할 선택된 아이템의 전체 구조체 정보 */
	UPROPERTY(BlueprintReadOnly, Category = "FT|Inventory")
	FFTInventoryItem SelectedItemDetail;

	/** @brief 현재 상세 선택된 슬롯의 카테고리 내 인덱스 */
	UPROPERTY(BlueprintReadOnly, Category = "FT|Inventory")
	int32 SelectedItemIndex = INDEX_NONE;

	/** @brief 현재 선택되어 있는 카테고리 필터링 탭 */
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	EFTItemCategoryType CurrentCategory = EFTItemCategoryType::None;

	/** @brief 데이터 변경 시 UI에 전파할 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "FT|Inventory")
	FFTOnViewModelChanged OnViewModelChanged;

	/** @brief 연동된 인벤토리 컴포넌트 반환 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	UFTInventoryComponent* GetLinkedInventory() const { return LinkedInventory.Get(); }

public:
	/** @brief 뷰모델 데이터를 인벤토리 컴포넌트의 실제 최신값으로 강제 동기화 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void NotifyChanged();

	/**
	 * @brief 상단 카테고리 탭 변경
	 * @param NewCategory : 새로 설정할 필터 카테고리
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetCategoryTab(EFTItemCategoryType NewCategory);

	/**
	 * @brief 특정 슬롯을 클릭했을 때 우측 상세 패널 정보 갱신
	 * @param SlotIndex : 선택된 아이템의 카테고리 내 인덱스
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SelectItemDetailAtIndex(int32 SlotIndex);

	/**
	 * @brief 특정 슬롯 체크박스의 클릭 상태(체크 여부) 기록
	 * @param SlotIndex : 선택 상태를 설정할 아이템의 카테고리 내 인덱스
	 * @param bIsSelected : 선택(체크) 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetItemSelection(int32 SlotIndex, bool bIsSelected);

	/** @brief 다중 선택 상태 및 상세 패널 선택 정보 초기화 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void ClearSelection();

	/**
	 * @brief 현재 체크되어 있는 아이템 수량 반환
	 * @return 현재 체크 선택된 아이템의 총 종류 수
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	int32 GetSelectedCount() const { return SelectedIndices.Num(); }

	/**
	 * @brief 선택 버리기 버튼의 활성화/비활성화 여부 결정
	 * @return 버리기 버튼의 활성화 가능 여부 (선택된 아이템이 1개 이상이면 true)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	bool IsDiscardButtonEnabled() const { return SelectedIndices.Num() > 0; }

	/**
	 * @brief 특정 슬롯 인덱스가 다중 선택(체크)되어 있는지 확인
	 * @param SlotIndex : 확인할 인벤토리 슬롯 인덱스
	 * @return 선택되어 있으면 true
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	bool IsIndexSelected(int32 SlotIndex) const;

	/** @brief 체크 선택된 다중 아이템들을 드롭 및 일괄 파괴 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void DiscardSelectedItems();

	/**
	 * @brief 무게 비율에 게이지바 전용 색상 반환
	 * @return 진행률에 따른 게이지바 색상 (90%이상 빨강 / 70%이상 노랑 / 일반 초록)
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	FLinearColor GetWeightBarColor() const;

	/**
	 * @brief 포맷팅된 무게 텍스트 반환 (예: "15.6 / 40.0 kg")
	 * @return 포맷팅된 무게 텍스트 FText
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	FText GetWeightText() const;

	/**
	 * @brief 무게 진행률 바인딩 비율 반환 (0.0 ~ 1.0)
	 * @return 무게 진행률 비율
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	float GetWeightPercent() const;

private:
	/** @brief 연동된 인벤토리 컴포넌트 약한 참조 */
	UPROPERTY(Transient)
	TWeakObjectPtr<UFTInventoryComponent> LinkedInventory;

	/** @brief 다중 선택(체크)된 실제 인벤토리 슬롯 인덱스 모음 */
	TSet<int32> SelectedIndices;
};
