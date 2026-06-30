#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_LauncherAction.generated.h"

class UFTItemDataAsset;
class UFTLauncherDataAsset;
class UFTProjectileActorDataAsset;
class UMeshComponent;

struct FFTLauncherActionStruct;
struct FFTProjectileActorStruct;

/**
 * 발사기류 아이템을 즉시 발사하는 어빌리티다.
 *
 * 입력 흐름:
 * - Event.UseItem으로 발동한다.
 * - Launcher DataAsset에서 발사할 ProjectileActorDataAsset을 찾는다.
 * - 인벤토리에 탄환 아이템이 있는지 확인하고, 성공 시 ProjectileActor를 스폰한다.
 * - 스폰된 ProjectileActor가 충돌하면 UFTGA_ProjectileAction을 Event.TargetHit으로 실행한다.
 */
UCLASS()
class PROJECTFT_API UFTGA_LauncherAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_LauncherAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	// 발사기 소켓/카메라 방향 기준으로 ProjectileActor를 스폰하고 초기 속도를 부여한다.
	bool FireProjectile();

	// ProjectileActor가 적중 이벤트를 보낼 수 있도록 ProjectileAction 어빌리티를 ASC에 보장한다.
	bool EnsureProjectileAbilityGranted();

	// 발사기 어빌리티 종료 공통 처리와 활성 데이터 캐시 정리.
	void EndLauncherAbility(bool bWasCancelled);

	// 현재 활성화된 Launcher DataAsset에서 발사기 설정만 꺼낸다.
	const FFTLauncherActionStruct* GetLauncherActionData() const;

	// Launcher DataAsset이 참조하는 ProjectileActorDataAsset에서 투사체 설정만 꺼낸다.
	const FFTProjectileActorStruct* GetProjectileActorData() const;

	// 캐릭터에게 장착된 액터들 중 Muzzle 소켓을 가진 Mesh를 찾는다.
	UMeshComponent* ResolveLauncherMesh(
		AActor* Avatar,
		FName RequiredSocketName
	) const;

private:
	// 이번 활성에서 사용 중인 원본 아이템 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	// ActiveItemData를 발사기 데이터로 캐스팅한 캐시.
	UPROPERTY(Transient)
	TObjectPtr<UFTLauncherDataAsset> ActiveLauncherData = nullptr;

	// 실제로 스폰할 ProjectileActor의 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;
};
