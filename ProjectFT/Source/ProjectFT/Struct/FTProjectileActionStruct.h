#pragma once

#include "CoreMinimal.h"
#include "FTProjectileActionStruct.generated.h"

class UAnimMontage;
class UFTItemDataAsset;

/**
 * 예전 ProjectileAction 데이터 구조다.
 *
 * 현재 실제 투사체 스폰/충돌/폭발 설정은 FFTProjectileActorStruct와
 * UFTProjectileActorDataAsset 쪽에서 주로 처리한다.
 * 이 구조체를 참조하는 기존 에셋이 있을 수 있으므로 필드는 유지한다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTProjectileActionStruct
{
	GENERATED_BODY()

public:
	// Projectile 액션에서 재생할 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 아이템을 손에 장착할 때 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");

	// Projectile로 사용할 아이템 데이터 에셋.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ActorData")
	TObjectPtr<UFTItemDataAsset> ItemData;

	// Projectile에 전달할 초기 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	float ProjectileSpeed = 3000.0f;

	// 효과를 적용할 수 있는 최소 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	float MinDamageSpeed = 1000.f;

	// true면 한 대상에게 한 번만 효과를 적용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	bool bDamageOnlyOnce = true;

	// true면 충돌 후 ProjectileActor를 제거한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	bool bDestroyOnImpact = false;
};
