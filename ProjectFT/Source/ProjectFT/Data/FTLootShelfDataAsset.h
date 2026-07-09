#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FTLootShelfDataAsset.generated.h"

class UStaticMesh;
class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct FFTLootShelfItemRow
{
	GENERATED_BODY()

	/* @brief : 획득 가능한 아이템 데이터 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<class UFTItemDataAsset> ItemDataAsset;

	/* @brief : 드롭 가중치 (이 값이 높을수록 획득 확률 증가. 예: 100 = 흔함, 5 = 희귀) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 Weight = 100;
};

/**
 * 매대 종류별 데이터 에셋.
 * 매대 형태(스태틱 메시), 획득 가능한 아이템 목록, 내구도, 상호작용 속성 등을 정의한다.
 */
UCLASS(BlueprintType)
class PROJECTFT_API UFTLootShelfDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/* @brief : 매대의 기본 스태틱 메시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<UStaticMesh> ShelfMesh;

	/* @brief : 매대가 상호작용 쿨다운 중일 때 표시할 대체 메시 (예: 텅 빈 상태) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<UStaticMesh> CooldownMesh;

	/* @brief : 매대 내구도 (파괴용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxHealth = 30.0f;

	/* @brief : 상호작용에 필요한 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.01"))
	float RequiredSeconds = 3.0f;

	/* @brief : 상호작용 성공 후 재사용 대기시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 10.0f;

	/* @brief : 상호작용 시 표시할 기본 프롬프트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionPrompt = FText::FromString(TEXT("훔치기"));

	/* @brief : 상호작용 쿨다운 중일 때 표시할 프롬프트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText CooldownPrompt = FText::FromString(TEXT("재충전 중..."));

	/* @brief : 매대에서 획득 가능한 아이템 목록 및 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FFTLootShelfItemRow> PossibleLootItems;

	/* @brief : 한 번에 획득/드롭할 아이템 최소 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 LootQuantityMin = 1;

	/* @brief : 한 번에 획득/드롭할 아이템 최대 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 LootQuantityMax = 3;

	/* @brief : 상호작용(훔치기) 완료 시 매대를 즉시 제거할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bDestroyOnComplete = false;

	/*
	 * @brief : 에셋 매니저 시스템에서 식별을 위한 고유 ID를 반환합니다.
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FName("FTLootShelf"), GetFName());
	}
};
