#pragma once

#include "CoreMinimal.h"
#include "FTGameplayAbility.h"
#include "FTGA_MeleeAttack.generated.h"

class UFTItemDataAsset;
class UFTMeleeDataAsset;
struct FFTMeleeAttackStruct;

// 근접 공격 Ability다.
//
// 책임:
// - Payload로 넘어온 UFTMeleeDataAsset을 현재 공격 데이터로 보관한다.
// - MeleeAttackData.AttackMontage를 재생한다.
// - AnimNotifyState가 열어준 타격 구간 동안 소켓 기반 캡슐 오버랩을 수행한다.
// - 한 대상에게 한 번만 ItemData.UseData.UseEffects를 적용한다.
UCLASS()
class PROJECTFT_API UFTGA_MeleeAttack : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_MeleeAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	// AnimNotifyState의 NotifyBegin에서 호출하는 판정 시작 함수다.
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void StartMeleeTrace();

	// AnimNotifyState의 NotifyTick에서 호출하는 실제 판정 함수다.
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void PerformMeleeTrace();

	// AnimNotifyState의 NotifyEnd에서 호출하는 판정 종료 함수다.
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void StopMeleeTrace();

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	void EndMeleeAbility(bool bWasCancelled);

	const FFTMeleeAttackStruct* GetMeleeAttackData() const;
	USkeletalMeshComponent* ResolveSourceMesh() const;
	bool BuildTraceCapsule(USkeletalMeshComponent* SourceMesh, FVector& OutStart, FVector& OutEnd, float& OutHalfHeight, FQuat& OutRotation) const;
	void ApplyItemEffectsToTarget(AActor* TargetActor) const;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTMeleeDataAsset> ActiveMeleeData = nullptr;

	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> HitActors;

	bool bTraceActive = false;
};
