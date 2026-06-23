// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "FTInventoryComponent.generated.h"

// 인벤토리 슬롯 구조체
USTRUCT(BlueprintType)
struct FFTInventoryItem
{
	GENERATED_BODY()

public:
	// 아이템 고유 식별자 (DataAsset의 ItemId와 일치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemId = NAME_None;

	// 해당 슬롯에 누적된 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	// 아이템의 세부 메타데이터 (이름, 아이콘, 무게 등) 정보 포인터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UFTItemDataAsset> ItemDataAsset = nullptr;
};

// 인벤토리 상태 변경 시 UI 알림용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnInventoryChanged);

UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTInventoryComponent();

	//~ 인벤토리 조작 API
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	bool AddItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	bool RemoveItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void ClearInventory();

	//~ Getters
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	const TArray<FFTInventoryItem>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	float GetCurrentWeight() const { return CurrentWeight; }

	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	float GetMaxWeight() const { return MaxWeight; }

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetMaxWeight(float NewMaxWeight);

	//~ 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "FT|Inventory")
	FFTOnInventoryChanged OnInventoryChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 무게 및 스탯 갱신
	void UpdateWeight();

public:
	// ItemId를 통해 메타데이터를 검색하는 헬퍼 함수
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	UFTItemDataAsset* FindItemData(FName ItemId) const;

protected:
	// GameplayMessage 수신 처리기
	void HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);

protected:
	// 인벤토리에 들어 있는 실제 아이템 리스트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Inventory")
	TArray<FFTInventoryItem> Items;

	// 최대 소지 가능 무게
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Inventory|Capacity")
	float MaxWeight = 100.0f;

	// 현재 총 무게
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Inventory|Capacity")
	float CurrentWeight = 0.0f;

private:
	// GameplayMessageSubsystem 구독 해제용 핸들
	FGameplayMessageListenerHandle MessageListenerHandle;
};
