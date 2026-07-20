#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/NPC/FTCashierAIController.h"
#include "FTStaffAIController.generated.h"

struct FFTMessagePayloadStruct;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Staff|Debug")
	bool bLogStaffDebug = false;

	/** 직원이 매대 앞에 도착했을 때 호출해 재보충 요청 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Restock")
	bool BroadcastRestockRequested();

	/** 현재 재보충 타겟을 비운다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Staff|Restock")
	void ClearRestockTarget();

private:
	FGameplayMessageListenerHandle StealCompletedListenerHandle;
	FGameplayMessageListenerHandle ShelfRestockedListenerHandle;

	void OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void OnShelfRestocked(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
};
