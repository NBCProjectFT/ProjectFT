#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTProjectileActor.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS(Blueprintable)
class PROJECTFT_API AFTProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	AFTProjectileActor();
	void InitializeProjectile(float InDamage,
		UAbilitySystemComponent* InSourceAbilitySystem = nullptr,
		TSubclassOf<UGameplayEffect> InEffectClass = nullptr,
		float InSpeed = 3000.0f,
		float InLifeSpan = 5.0f,
		float InGravityScale = 0.0f,
		float InCollisionRadius = 8.0f);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Projectile")
	float Damage = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> EffectClass;
};
