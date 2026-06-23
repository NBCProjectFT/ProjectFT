#include "FTProjectileComponent.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Weapon/Projectile/FTProjectileActor.h"

UFTProjectileComponent::UFTProjectileComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AFTProjectileActor* UFTProjectileComponent::SpawnProjectile(
	const FFTItemActionDefinition& ActionDefinition,
	const FTransform& SpawnTransform,
	UAbilitySystemComponent* SourceAbilitySystem,
	float Damage,
	FVector LaunchDirection,
	APawn* InstigatorPawn) const
{
	AFTItemActor* OwnerItem = Cast<AFTItemActor>(GetOwner());
	if (!OwnerItem || !GetWorld())
	{
		return nullptr;
	}

	TSubclassOf<AFTProjectileActor> ProjectileClass = ActionDefinition.ProjectileClass;
	if (!ProjectileClass)
	{
		ProjectileClass = AFTProjectileActor::StaticClass();
	}

	AFTProjectileActor* Projectile = GetWorld()->SpawnActorDeferred<AFTProjectileActor>(
		ProjectileClass,
		SpawnTransform,
		OwnerItem,
		InstigatorPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return nullptr;
	}

	UFTItemDataAsset* ProjectileItemData = ActionDefinition.ProjectileItemData
		? ActionDefinition.ProjectileItemData.Get()
		: nullptr;
	const float CollisionScale = ProjectileItemData
		? ProjectileItemData->ProjectileCollisionScale
		: 1.0f;
	UE_LOG(LogFTItem, Log,
		TEXT("Projectile component spawned ProjectileActor: %s (ProjectileItemData: %s, CollisionScale: %.2f)"),
		*GetNameSafe(Projectile),
		*GetNameSafe(ProjectileItemData),
		CollisionScale);

	Projectile->InitializeProjectile(
		Damage,
		SourceAbilitySystem,
		ActionDefinition.EffectClass,
		ProjectileItemData,
		LaunchDirection,
		ActionDefinition.ProjectileSpeed,
		ActionDefinition.ProjectileLifeSpan,
		ActionDefinition.ProjectileGravityScale,
		FMath::Max(1.0f, ActionDefinition.ProjectileCollisionRadius * CollisionScale));
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	return Projectile;
}
