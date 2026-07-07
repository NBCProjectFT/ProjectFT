#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "FTItemPoolSubsystem.generated.h"

class AFTItemActor;
class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct FFTItemActorArray
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<AFTItemActor>> Actors;
};

/**
 * @brief 아이템 풀링 및 필드 드롭 아이템 스폰을 전역 관리하는 월드 서브시스템
 */
UCLASS()
class PROJECTFT_API UFTItemPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/**
	 * @brief 풀에서 아이템 액터를 획득하거나 새로 생성합니다.
	 * @param ItemId : 생성할 아이템 ID
	 * @param Location : 스폰할 월드 위치
	 * @param Rotation : 스폰할 월드 회전값
	 * @return 생성/획득된 아이템 액터 포인터
	 */
	AFTItemActor* AcquireItemActor(FName ItemId, const FVector& Location, const FRotator& Rotation);

	/**
	 * @brief 활성화된 아이템 액터를 반환받아 풀에 넣고 비활성화합니다.
	 * @param ItemActor : 반환할 아이템 액터 포인터
	 */
	void ReleaseItemActor(AFTItemActor* ItemActor);

protected:
	/** @brief 드롭 요청 메시지 수신 시 처리할 핸들러 */
	void HandleDropItemMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);

	/** @brief 플레이어 주변 적절한 드롭 스폰 위치 계산 */
	FVector CalculateDropLocation(AActor* InstigatorActor) const;

	/** @brief 아이템 ID로 데이터 에셋 탐색 */
	UFTItemDataAsset* FindItemData(FName ItemId) const;

private:
	/** @brief 드롭 요청 메시지 리스너 핸들 */
	FGameplayMessageListenerHandle DropItemListenerHandle;

	/** @brief 비활성화되어 재사용 대기 중인 아이템 액터 풀 (아이템 ID별로 나누어 관리) */
	UPROPERTY()
	TMap<FName, FFTItemActorArray> InactivePoolsMap;


};
