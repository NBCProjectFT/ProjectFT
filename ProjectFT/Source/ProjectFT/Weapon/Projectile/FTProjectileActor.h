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
		TSubclassOf<UGameplayEffect> InDamageEffectClass = nullptr);

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
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
