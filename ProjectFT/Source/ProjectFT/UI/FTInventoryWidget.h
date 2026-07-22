#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInventoryWidget.generated.h"

class USoundBase;

/** @brief 인벤토리 UI 연동 및 뷰모델 이벤트를 처리하는 위젯 클래스 */
UCLASS()
class PROJECTFT_API UFTInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** @brief 위젯 생성 시 초기화 및 뷰모델 이벤트 바인딩을 처리 */
	virtual void NativeConstruct() override;

	/** @brief 위젯 제거 시 클린업 및 사운드 재생을 처리 */
	virtual void NativeDestruct() override;

	/** @brief 연동된 인벤토리 뷰모델 객체 포인터 */
	UPROPERTY(BlueprintReadOnly, Category = "FT|Inventory")
	TObjectPtr<class UFTInventoryViewModel> ViewModel;

	/** @brief 뷰모델의 데이터가 갱신되었을 때 호출되어 UI를 업데이트합니다. */
	UFUNCTION()
	void HandleViewModelChanged();

private:
	bool bHasConstructed = false;

public:
	/**
	 * @brief 인벤토리 컴포넌트를 전달받아 뷰모델을 설정하고 바인딩을 수행합니다.
	 * @param InInventoryComponent : 대상 인벤토리 컴포넌트
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetupInventory(class UFTInventoryComponent* InInventoryComponent);

	/** @brief 아이템 목록 UI 리스트를 최신 데이터로 리프레시하는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Inventory")
	void RefreshItemList();

	/**
	 * @brief 무게 정보가 변경되었을 때 게이지 및 텍스트를 업데이트하는 이벤트
	 * @param CurrentWeight : 현재 총 무게
	 * @param MaxWeight : 최대 소지 무게
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Inventory")
	void UpdateWeight(float CurrentWeight, float MaxWeight);

	/**
	 * @brief 특정 아이템 슬롯이 선택되었을 때 상세 정보를 표시하는 이벤트
	 * @param ItemId : 선택된 아이템의 ID
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Inventory")
	void ShowItemDetail(FName ItemId);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory|Audio")
	TObjectPtr<USoundBase> OpenSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Inventory|Audio")
	TObjectPtr<USoundBase> CloseSound = nullptr;
};
