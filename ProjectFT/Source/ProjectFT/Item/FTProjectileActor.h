#pragma once

#include "CoreMinimal.h"
#include "FTItemActor.h"
#include "FTProjectileActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UFTProjectileActorDataAsset;

UCLASS()
class PROJECTFT_API AFTProjectileActor : public AFTItemActor
{
	GENERATED_BODY()

public:
	AFTProjectileActor();

	void InitializeProjectile(
		UFTProjectileActorDataAsset* InProjectileActorData,
		const FVector& FireDirection,
		AActor* InOwnerActor
	);

	void InitializeHeldProjectile(
		UFTProjectileActorDataAsset* InProjectileActorData,
		AActor* InOwnerActor
	);

	void ReleaseProjectile(const FVector& FireDirection);

protected:
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> ProjectileCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitActors;

	UPROPERTY(Transient)
	bool bHasExploded = false;

	// 마지막으로 착탄음을 낸 월드 시각. ImpactSoundMinInterval 판정용
	// (월드 시각은 0부터 시작하므로, 큰 음수로 두면 첫 충돌은 간격 검사를 항상 통과한다).
	float LastImpactSoundTime = -1000.0f;

	// 데이터의 착탄음을 현재 위치에서 재생한다. 최소 간격 안이면 건너뛴다.
	void PlayImpactSound();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	float MinDamageSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bDamageOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bDestroyOnImpact = false;

	UFUNCTION()
	void OnProjectileBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	void ConfigureProjectileCollision(const struct FFTProjectileActorStruct& ProjectileData);

	void HandleProjectileImpact(AActor* HitActor);

	void Explode(AActor* DirectHitActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void ReceiveExplode(const FVector& ExplosionLocation);

	void SendTargetHitEvent(AActor* TargetActor);

	bool IsValidDirectHitTarget(AActor* TargetActor) const;

public:
	virtual void Tick(float DeltaTime) override;
};
