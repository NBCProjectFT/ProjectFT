// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubStorage.generated.h"

class UFTInventoryComponent;

/**
 * @brief 허브에 배치되는 창고 상호작용 액터.
 *
 * 이 액터는 월드에 존재하는 "창고 입구" 역할만 맡는다.
 * 실제 아이템 보관 데이터는 StorageInventory 컴포넌트에 있고,
 * 아이템 추가/제거 규칙은 UFTStorageSubsystem이 처리한다.
 *
 * @see UFTStorageSubsystem
 * @see UFTHubStorageWidget
 * @see UFTHubStorageViewModel
 */
UCLASS()
class PROJECTFT_API AFTHubStorage : public AActor,  public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubStorage();

	/**
	 * @brief 창고가 사용하는 인벤토리 컴포넌트를 반환한다.
	 *
	 * ViewModel, 작업대, 퀘스트 같은 허브 시스템은 이 함수를 통해
	 * 창고 안의 아이템을 조회하거나 소모 요청에 넘긴다.
	 */
	UFTInventoryComponent* GetStorageInventory() const;
	
	/**
	 * @brief 플레이어가 창고와 상호작용했을 때 호출된다.
	 *
	 * 액터는 직접 UI를 만들지 않고 UIManager에 창고 UI 열기를 위임한다.
	 */
	virtual bool Interact_Implementation(AActor* Interactor) override;

	/**
	 * @brief 상호작용 안내 문구를 반환한다.
	 */
	virtual FText GetInteractionPrompt_Implementation() const override;

	/**
	 * @brief 창고 UI 닫기 요청을 UIManager로 전달한다.
	 *
	 * 창고 위젯의 닫기 버튼에서 다시 액터를 통해 호출되는 진입점이다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage|UI")
	void CloseStorageWidget();

protected:
	virtual void BeginPlay() override;

private:
	/**
	 * @brief 창고의 실제 아이템 목록을 담는 인벤토리 컴포넌트.
	 *
	 * 창고는 별도 저장 구조를 만들지 않고 기존 인벤토리 컴포넌트를 재사용한다.
	 */
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="Storage",meta = (AllowPrivateAccess = "true"))
	UFTInventoryComponent * StorageInventory;
	
	/**
	 * @brief 테스트/초기 배치용 창고 아이템 목록.
	 *
	 * BeginPlay에서 UFTStorageSubsystem::InitializeStorage로 전달된다.
	 * 실제 밸런스 데이터로 확장할 때는 DataAsset/DataTable 분리를 고려한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Storage|Test")
	TArray<FTStorageItemStruct> TestStorageItems;

	/**
	 * @brief UIManager를 찾아 창고 UI를 연다.
	 *
	 * @param Interactor 창고와 상호작용한 액터. 플레이어 인벤토리를 찾는 데 사용된다.
	 */
	void OpenStorageWidget(AActor* Interactor);
};
