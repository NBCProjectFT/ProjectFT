#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTStaffRestockManager.generated.h"

struct FFTMessagePayloadStruct;

UCLASS()
class PROJECTFT_API AFTStaffRestockManager : public AActor
{
	GENERATED_BODY()

public:
	AFTStaffRestockManager();

	/** 빈 매대를 대기 목록에 등록합니다. */
	void RegisterEmptyShelf(AActor* ShelfActor);

	/** 아직 다른 직원에게 예약되지 않은 빈 매대를 직원에게 할당합니다. */
	bool TryAssignShelf(AActor* StaffActor, AActor*& OutShelfActor);

	/** 재보충이 끝난 매대를 대기 목록과 예약 목록에서 제거합니다. */
	void CompleteShelf(AActor* ShelfActor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Staff|Debug")
	bool bLogRestockDebug = false;

private:
	TArray<TWeakObjectPtr<AActor>> PendingShelves;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> AssignedShelves;
	FGameplayMessageListenerHandle StealCompletedListenerHandle;

	void OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void RemoveInvalidShelves();
};
