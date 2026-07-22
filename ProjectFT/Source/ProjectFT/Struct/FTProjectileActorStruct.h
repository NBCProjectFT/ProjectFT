#pragma once

#include "CoreMinimal.h"
#include "FTProjectileActorStruct.generated.h"

class AFTProjectileActor;
class USoundBase;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTProjectileActorStruct
{
	GENERATED_BODY()

public:
	// 실제 스폰할 Projectile Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Actor")
	TSubclassOf<AFTProjectileActor> ProjectileActorClass;
	
	// true면 Projectile 자체가 Overlap 충돌을 사용한다.
	// false면 기본적으로 Block 충돌을 사용하고 OnHit으로 처리한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
	bool bUseOverlapCollision = false;
	
	// 직접 충돌했을 때 효과 적용 대상으로 인정할 Object Channel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
	TEnumAsByte<ECollisionChannel> DirectHitTargetObjectChannel = ECC_Pawn;

	// 폭발 범위에서 찾을 대상 Object Channel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	TEnumAsByte<ECollisionChannel> ExplosionTargetObjectChannel = ECC_Pawn;
	
	// 충돌 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision", meta = (ClampMin = "1.0"))
	float CollisionRadius = 20.0f;
	
	// Projectile에 전달할 초기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 3000.0f;

	// ProjectileMovementComponent의 최대 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 3000.0f;

	// ProjectileMovementComponent의 중력 영향
	// 0.0 = 직선에 가까운 발사체, 1.0 = 일반 중력 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
	float GravityScale = 0.0f;

	// Projectile 수명 (0.0f는 수명 무한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Life", meta = (ClampMin = "0.0"))
	float LifeTime = 0.0f;

	// 데미지를 가하는 상태의 최소 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact", meta = (ClampMin = "0.0"))
	float MinDamageSpeed = 1000.0f;

	// 한 대상에게 한 번만 데미지를 줄지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bDamageOnlyOnce = true;

	// 적이나 벽에 닿은 후 Destroy되는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bDestroyOnImpact = false;
	
	// 충돌 대상에게 효과 적용 이벤트를 보낼지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bApplyEffectsOnImpact = true;
	
	// 폭발형 투사체인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bExplodeOnImpact = false;
	
	// 수명이 끝났을 때 폭발할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bExplodeOnLifeEnd = false;
	
	// 폭발 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 300.0f;
	
	// 폭발 시 자기 자신/소유자를 제외할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bIgnoreOwnerInExplosion = true;
	
	// 폭발 범위 안 대상들에게 효과 적용 이벤트를 보낼지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bApplyEffectsToExplosionTargets = true;
	
	// 폭발 후 Destroy할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bDestroyAfterExplosion = true;

	// 무언가에 부딪힌 순간 충돌 지점에서 재생할 착탄음. 비우면 무음.
	// 폭발한 경우에는 재생되지 않는다 — 그쪽은 아래 ExplosionSound가 대신한다(툭 소리와 폭발음이 겹치지 않게).
	// 던지는 소리는 이게 아니라 아이템 데이터의 UseData.UseSound다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Audio")
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	// 폭발할 때(bExplodeOnImpact/bExplodeOnLifeEnd) 재생할 소리. 폭발형이 아니면 비워둔다.
	// 폭발은 bHasExploded 가드로 1회만 일어나므로 이 소리도 자연히 한 번만 난다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Audio")
	TObjectPtr<USoundBase> ExplosionSound = nullptr;

	// 착탄음 최소 간격(초). 한 프레임에 여러 대상과 겹치거나(Overlap 모드) 연속으로 부딪힐 때
	// 같은 소리가 뭉쳐 터지는 것을 막는다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Audio", meta = (ClampMin = "0.0"))
	float ImpactSoundMinInterval = 0.1f;
};