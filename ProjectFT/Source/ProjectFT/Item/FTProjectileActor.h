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

	void SendTargetHitEvent(AActor* TargetActor);

	bool IsValidDirectHitTarget(AActor* TargetActor) const;

public:
	virtual void Tick(float DeltaTime) override;
};
