#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "FTLootShelf.generated.h"

class UStaticMeshComponent;
class UFTChanneledInteractionComponent;
class USoundBase;
class UAudioComponent;

/**
 * 마트 진열대에서 물건을 "훔치는" 채널형 상호작용 대상(테스트/예시).
 * 상호작용 키를 꾹 누르면 게이지가 차고 중간중간 스킬체크가 뜨며, 게이지가 가득 차면 훔치기 완료로 처리한다.
 * 채널 로직은 UFTChanneledInteractionComponent가, 포커스/프롬프트는 IFTInteractable이 담당한다.
 */
UCLASS()
class PROJECTFT_API AFTLootShelf : public AActor, public IFTInteractable, public IFTDamageable
{
	GENERATED_BODY()

public:
	AFTLootShelf();

	/* @brief : UI 프롬프트에 표시할 텍스트를 반환합니다. */
	virtual FText GetInteractionPrompt_Implementation() const override;

	/* @brief : 매대에 데미지를 가합니다. 체력이 0 이하가 되면 파괴됩니다.  */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/* @brief : 에셋 할당 및 컴포넌트 변수 설정을 처리하는 생성 시점의 메서드입니다. */
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "FT|Shelf|Status")
	float GetHealthPercent() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/* @brief : 채널형 상호작용(훔치기)이 완료되었을 때 호출되는 메서드입니다. */
	UFUNCTION()
	void HandleStealCompleted();

	/* @brief : 채널형 상호작용 진행 상태가 변경될 때 호출되는 메서드입니다. */
	UFUNCTION()
	void HandleChannelStateChanged(bool bIsChanneling);

	/* @brief : 훔치기 완료 시 인벤토리에 아이템을 직접 보상으로 지급합니다. */
	void GiveStealReward();

	/* @brief : 매대 파괴 시 아이템들을 주변 바닥에 드롭시킵니다. */
	void DropItemsOnFloor();

	/* @brief : 지정된 데이터 에셋에 맞게 매대의 스태틱 메시 및 내구도 설정을 초기화합니다. */
	void InitializeFromDataAsset();

	/*
	 * @brief : 매대를 상호작용 쿨다운 상태로 전환합니다.
	 */
	void StartInteractionCooldown();

	/* @brief : 매대의 상호작용 쿨다운 상태를 해제하고 초기 상태로 복구합니다. */
	void EndInteractionCooldown();

	/* @brief : 재입고 요청 메시지를 처리하는 콜백입니다. */
	void HandleRestockRequested(FGameplayTag Channel, const struct FFTMessagePayloadStruct& Payload);

	/*
	 * @brief : 매대에 할당된 아이템 데이터 풀에서 랜덤하게 아이템 및 수량을 선정합니다.
	 * @Param OutQuantity : 선정된 아이템 수량 반환용 참조 변수
	 */
	class UFTItemDataAsset* GetRandomLootItem(int32& OutQuantity) const;

	/* @brief : 가중치를 고려하여 매대 아이템 풀에서 임의의 단일 아이템 에셋을 선택합니다. */
	class UFTItemDataAsset* SelectRandomItemFromPool() const;

	/*
	 * @brief : 월드 상에 실제로 획득 가능한 아이템 액터를 물리 속성을 포함해 스폰합니다.
	 * @Param ItemDataAsset : 스폰할 아이템의 데이터 에셋
	 */
	void SpawnItemActor(class UFTItemDataAsset* ItemDataAsset);

protected:
	/* @brief : 진열대 스태틱 메시 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shelf", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShelfMesh;

	/* @brief : 채널형 상호작용 처리를 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shelf", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTChanneledInteractionComponent> ChanneledInteraction;

	/* @brief : 매대 데이터 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Data")
	TObjectPtr<class UFTLootShelfDataAsset> ShelfDataAsset;

	/* @brief : 현재 매대의 내구도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Status")
	float Health = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Status")
	float MaxHealth = 30.0f;

	/* @brief : 상호작용 및 파괴 완료 상태 여부 */
	bool bHasBeenLooted = false;

	/* @brief : 현재 상호작용 쿨다운(재충전) 상태인지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shelf|Cooldown")
	bool bIsOnCooldown = false;

	/* @brief : 쿨다운 해제를 위한 타이머 핸들 */
	FTimerHandle CooldownTimerHandle;

	/* @brief : 재입고 요청 감지를 위한 메시지 리스너 핸들 */
	FGameplayMessageListenerHandle RestockRequestListenerHandle;

protected:
	/* @brief : 매대를 부실 때(타격 시) 재생할 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shelf|Audio")
	TObjectPtr<USoundBase> DamagedSound = nullptr;

	/* @brief : 매대가 완전히 부서졌을 때 재생할 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shelf|Audio")
	TObjectPtr<USoundBase> DestroyedSound = nullptr;

	/* @brief : 매대 상호작용 중일 때(루프 등) 재생할 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shelf|Audio")
	TObjectPtr<USoundBase> InteractSound = nullptr;

	/* @brief : 매대 재입고 완료(다시 채울 때) 시 재생할 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Shelf|Audio")
	TObjectPtr<USoundBase> RestockedSound = nullptr;

	/* @brief : 현재 재생 중인 상호작용 지속 사운드 컴포넌트 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveAudioComponent = nullptr;
};
