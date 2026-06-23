#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTProjectileActor.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class UFTItemDataAsset;

UCLASS(Blueprintable)
class PROJECTFT_API AFTProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	AFTProjectileActor();

	void InitializeProjectile(float InDamage,
		UAbilitySystemComponent* InSourceAbilitySystem = nullptr,
		TSubclassOf<UGameplayEffect> InEffectClass = nullptr,
		UFTItemDataAsset* InItemData = nullptr,
		FVector InLaunchDirection = FVector::ForwardVector,
		float InSpeed = 3000.0f,
		float InLifeSpan = 5.0f,
		float InGravityScale = 0.0f,
		float InCollisionRadius = 8.0f);

protected:
	virtual void BeginPlay() override;

	void InitializeFromItemData(UFTItemDataAsset* InItemData);
	void UpdateAppearance();
	void SpawnItemOnImpact(const FHitResult& Hit);

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Projectile")
	TObjectPtr<UFTItemDataAsset> ItemData;

	UPROPERTY(Transient)
	FVector LaunchDirection = FVector::ForwardVector;

	UPROPERTY(Transient)
	bool bHasImpacted = false;
};
