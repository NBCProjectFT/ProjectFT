#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/NPC/FTCashierAIController.h"
#include "FTStaffAIController.generated.h"

struct FFTMessagePayloadStruct;
class AFTShoppingPoint;

UCLASS()
class PROJECTFT_API AFTStaffAIController : public AFTCashierAIController
{
	GENERATED_BODY()

public:
	AFTStaffAIController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 재보충하러 이동할 매대다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Restock")
	TObjectPtr<AActor> TargetShelfActor;

	/** StateTree Move To에서 사용할 재보충 목적지다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Restock")
	FVector RestockLocation = FVector::ZeroVector;

	/** 현재 처리할 빈 매대가 있는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Restock")
	bool bHasRestockTarget = false;

	/** 현재 타겟 매대에 재보충 요청 메시지를 보냈는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Restock")
	bool bRestockRequested = false;

	/** 현재 타겟 매대의 재보충 완료 메시지를 받았는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Restock")
	bool bRestockCompleted = false;

	/** 비어있는 매대가 없을 때 직원이 이동할 대기 순찰 위치입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Wander")
	FVector StaffWanderLocation = FVector::ZeroVector;

	/** 현재 직원 대기 순찰 목적지가 있는지 나타냅니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Wander")
	bool bHasStaffWanderTarget = false;

	/** 직원 대기 순찰 Move To에서 도착으로 인정할 거리입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Staff|Wander")
	float StaffWanderAcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Staff|Debug")
	bool bLogStaffDebug = false;

	/** 직원이 매대 앞에 도착했을 때 호출해 재보충 요청 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Restock")
	bool BroadcastRestockRequested();

	/** 현재 재보충 타겟을 비운다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Restock")
	void ClearRestockTarget();

	/** 비어있는 매대가 없을 때 이동할 쇼핑 포인트 기반 대기 순찰 위치를 선택합니다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Wander")
	bool PickRandomStaffWanderTarget();

	/** 현재 선택한 대기 순찰 포인트 점유를 해제합니다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Wander")
	void ClearStaffWanderTarget();

private:
	FGameplayMessageListenerHandle StealCompletedListenerHandle;
	FGameplayMessageListenerHandle ShelfRestockedListenerHandle;

	UPROPERTY()
	TObjectPtr<AFTShoppingPoint> CurrentStaffWanderPoint;

	void OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void OnShelfRestocked(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
};
