#pragma once

#include "CoreMinimal.h"
#include "FTProjectileActionStruct.generated.h"

class UAnimMontage;
class UFTItemDataAsset;
class AFTProjectileActor;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTProjectileActionStruct
{
	GENERATED_BODY()

public:
	// 공격 때 재생할 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 어태치될 소켓의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");
	
	// 총구 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Launch Site")
	FName LaunchSocketName = TEXT("Muzzle");

	// 투사체 아이템 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileActor")
	TObjectPtr<AFTProjectileActor> ProjectileActor;
	
	// Projectile로 사용 될 아이템 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ActorData")
	TObjectPtr<UFTItemDataAsset> ItemData;
	
	// 사정거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|ProjectileData")
	float Range = 5000.0f;

	// Projectile에 전달할 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	float ProjectileSpeed = 3000.0f;

	// 데미지를 가하는 상태의 최소속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	float MinDamageSpeed = 1000.f;

	// 다른적에게 튕겨져서 닿으면 추가로 데미지를 가하는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	bool bDamageOnlyOnce = true;

	// 적에게 닿은 후 Destroy되는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|ProjectileData")
	bool bDestroyOnImpact = false;
};