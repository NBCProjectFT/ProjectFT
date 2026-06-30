#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_ThrowItemAction.generated.h"

class UFTItemDataAsset;
class UFTThrowDataAsset;
class UFTProjectileActorDataAsset;
class AFTProjectileActor;
class UMeshComponent;
class UAnimMontage;

struct FFTThrowActorStruct;
struct FFTProjectileActorStruct;
struct FGameplayEventData;

/**
 * 손에 투사체를 들었다가 입력을 놓는 타이밍에 던지는 아이템 어빌리티다.
 *
 * 흐름:
 * 1. Event.UseItem으로 발동한다.
 * 2. Throw DataAsset이 가리키는 ProjectileActorDataAsset으로 투사체 액터를 만들고 손 소켓에 붙인다.
 * 3. Event.UseReleased를 받으면 투척 몽타주를 재생한다.
 * 4. 몽타주 안의 UFTThrowReleaseAnimNotify가 Event.ThrowRelease를 보내면 실제로 발사한다.
 * 5. 발사체가 충돌하면 UFTGA_ProjectileAction이 TargetHit 이벤트로 효과를 적용한다.
 */
UCLASS()
class PROJECTFT_API UFTGA_ThrowItemAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_ThrowItemAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

private:
	// Throw DataAsset의 ProjectileActor를 생성해서 AttachSocketName 소켓에 붙인다.
	bool StartHoldingProjectile();

	// 손에 붙어 있던 ProjectileActor를 분리하고 카메라 방향으로 발사한다.
	bool ReleaseHeldProjectile();

	// ProjectileActor가 적중 이벤트를 보낼 수 있도록 ProjectileAction 어빌리티를 ASC에 보장한다.
	bool EnsureProjectileAbilityGranted();

	// 투척 어빌리티 종료 공통 처리. 취소 시 손에 든 투사체도 정리한다.
	void EndThrowAbility(bool bWasCancelled);

	// 사용 입력이 Released될 때까지 기다린다.
	bool WaitForUseReleased();

	// ThrowMontage 안의 ThrowRelease Notify 이벤트를 기다린다.
	bool WaitForThrowRelease();

	// 사용 입력을 놓으면 준비 상태에서 실제 투척 몽타주 단계로 넘어간다.
	UFUNCTION()
	void HandleUseReleasedEvent(FGameplayEventData Payload);

	// 몽타주 Notify가 던지는 프레임을 알려주면 실제 발사를 수행한다.
	UFUNCTION()
	void HandleThrowReleaseEvent(FGameplayEventData Payload);

	// ThrowRelease Notify 없이 몽타주가 끝나면 실패/취소로 처리한다.
	void HandleThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 손에 붙어 있는 임시 ProjectileActor를 제거한다.
	void ClearHeldProjectile();
	
	// 현재 활성화된 Throw DataAsset에서 투척 설정만 꺼낸다.
	const FFTThrowActorStruct* GetThrowActorData() const;

	// Throw DataAsset이 참조하는 ProjectileActorDataAsset에서 투사체 설정만 꺼낸다.
	const FFTProjectileActorStruct* GetProjectileActorData() const;
	
	// 캐릭터 본체나 장착 액터 중 RequiredSocketName을 가진 Mesh를 찾는다.
	UMeshComponent* ResolveAttachMesh(
		AActor* Avatar,
		FName RequiredSocketName
	) const;

	// 플레이어 카메라/컨트롤러 기준 발사 방향을 구한다.
	FVector GetViewDirection() const;

private:
	// 이번 활성에서 사용 중인 원본 아이템 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	// ActiveItemData를 투척 데이터로 캐스팅한 캐시.
	UPROPERTY(Transient)
	TObjectPtr<UFTThrowDataAsset> ActiveThrowData = nullptr;
	
	// 손에 들고 발사할 실제 ProjectileActor 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;

	// 아직 발사되지 않고 손 소켓에 붙어 있는 투사체 인스턴스.
	UPROPERTY(Transient)
	TObjectPtr<AFTProjectileActor> HeldProjectile = nullptr;

	// 현재 재생 중인 투척 몽타주. Notify 누락/몽타주 종료 fallback 판정에 사용한다.
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveThrowMontage = nullptr;
	
	// 손에 ProjectileActor를 들고 있는 상태인지.
	UPROPERTY(Transient)
	bool bIsHoldingProjectile = false;

	// ThrowRelease Notify를 기다리는 중인지.
	UPROPERTY(Transient)
	bool bWaitingForThrowRelease = false;
};
