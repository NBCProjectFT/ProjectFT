#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubStorageWidget.generated.h"

class AFTHubStorage;
class UButton;
class UFTHubStorageViewModel;
class UFTInventoryComponent;
class UListView;
class USpinBox;
class UTextBlock;
class UTileView;

/**
 * @brief 허브 창고 화면을 표시하는 UMG 위젯.
 *
 * 이 클래스는 버튼 클릭, ListView 선택, 텍스트 갱신 같은 View 책임을 가진다.
 * 아이템 이동 가능 여부나 실제 이동 명령은 UFTHubStorageViewModel에 위임한다.
 *
 * @see UFTHubStorageViewModel
 * @see AFTHubStorage
 */
UCLASS()
class PROJECTFT_API UFTHubStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 창고 위젯이 사용할 액터, 플레이어 인벤토리, ViewModel을 연결한다.
	 *
	 * UIManager가 창고 UI를 열 때 호출한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hub|Storage")
	void InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory, UFTHubStorageViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** @brief 플레이어 인벤토리 목록을 표시하는 TileView. */
	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_PlayerItems;

	/** @brief 창고 인벤토리 목록을 표시하는 TileView. */
	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_StorageItems;

	/** @brief 현재 선택 상태를 표시하는 텍스트. */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItem;

	/** @brief 플레이어 무게 정보를 표시하는 텍스트. */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PlayerWeight;

	/** @brief 이동 수량 선택용 SpinBox. 현재 구현에서는 선택 상태 표시/활성화 중심으로 사용된다. */
	UPROPERTY(meta = (BindWidgetOptional))
	USpinBox* SPB_MoveCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_MoveQuantity;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeQuantity;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityHalf;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityMax;

	/** @brief 선택된 플레이어 아이템을 창고로 넣는 버튼. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Store;

	/** @brief 선택된 창고 아이템을 플레이어에게 꺼내는 버튼. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Take;

	/** @brief 플레이어 아이템 전체를 창고로 넣는 버튼. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StoreAll;

	/** @brief 창고 아이템 전체를 플레이어에게 꺼내는 버튼. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_TakeAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterWeapon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterHealing;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterCommon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterWeapon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterHealing;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterCommon;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	/** @brief ViewModel의 현재 상태를 위젯 텍스트/목록/버튼 활성화에 반영한다. */
	UFUNCTION()
	void RefreshFromViewModel();

	/** @brief 플레이어 TileView를 ListView 기반 API로 다루기 위해 반환한다. */
	UListView* GetPlayerItemsView() const;

	/** @brief 창고 TileView를 ListView 기반 API로 다루기 위해 반환한다. */
	UListView* GetStorageItemsView() const;

	/** @brief ViewModel이 만든 아이템 객체 목록을 ListView에 다시 채운다. */
	void PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items);

	/** @brief 현재 ListView 선택 상태를 ViewModel의 선택 데이터로 전달한다. */
	void PushSelectedItemsToViewModel(UListView* ItemsView, bool bFromPlayerItems);

	void HandlePlayerItemClicked(UObject* Item);
	void HandleStorageItemClicked(UObject* Item);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleStoreClicked();

	UFUNCTION()
	void HandleTakeClicked();

	UFUNCTION()
	void HandleStoreAllClicked();

	UFUNCTION()
	void HandleTakeAllClicked();

	UFUNCTION()
	void HandleQuantityMinusClicked();

	UFUNCTION()
	void HandleQuantityPlusClicked();

	UFUNCTION()
	void HandleQuantityHalfClicked();

	UFUNCTION()
	void HandleQuantityMaxClicked();

	UFUNCTION()
	void HandlePlayerFilterAllClicked();

	UFUNCTION()
	void HandlePlayerFilterWeaponClicked();

	UFUNCTION()
	void HandlePlayerFilterHealingClicked();

	UFUNCTION()
	void HandlePlayerFilterCommonClicked();

	UFUNCTION()
	void HandleStorageFilterAllClicked();

	UFUNCTION()
	void HandleStorageFilterWeaponClicked();

	UFUNCTION()
	void HandleStorageFilterHealingClicked();

	UFUNCTION()
	void HandleStorageFilterCommonClicked();

	/** @brief 이 위젯이 닫기 요청을 전달할 창고 액터. */
	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage;

	/** @brief 창고 UI 상태와 명령을 담당하는 ViewModel. */
	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageViewModel> ViewModel;

	/** @brief 목록을 다시 채우는 중 발생하는 선택 이벤트를 무시하기 위한 가드. */
	bool bRefreshingFromViewModel = false;

	/** ListView 이벤트가 덮어쓰기 전 ViewModel에서 확정된 선택 상태. */
	TSet<TWeakObjectPtr<UObject>> PlayerSelectedItems;
	TSet<TWeakObjectPtr<UObject>> StorageSelectedItems;
};
