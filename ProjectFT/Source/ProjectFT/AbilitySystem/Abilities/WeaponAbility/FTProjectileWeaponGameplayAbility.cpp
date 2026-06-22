#include "FTProjectileWeaponGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Weapon/Projectile/FTProjectileActor.h"

bool UFTProjectileWeaponGameplayAbility::ExecuteWeaponAction()
{
	if (!ActiveItem || !ActiveDefinition || !ActiveDefinition->ProjectileClass || !GetWorld())
	{
		return false;
	}

	AActor* Shooter = GetAvatarActorFromActorInfo();
	if (!Shooter)
	{
		return false;
	}
	if (!Shooter->HasAuthority())
	{
		return true;
	}
	APawn* ShooterPawn = Cast<APawn>(Shooter);
	const FTransform MuzzleTransform = ActiveItem->GetMuzzleTransform();
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimOrigin = MuzzleLocation;
	FVector AimDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	if (const APlayerController* PC = ShooterPawn
		? Cast<APlayerController>(ShooterPawn->GetController()) : nullptr)
	{
		FRotator AimRotation;
		PC->GetPlayerViewPoint(AimOrigin, AimRotation);
		AimDirection = AimRotation.Vector();
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FT_GAS_ProjectileAim), true);
	Params.AddIgnoredActor(ActiveItem);
	Params.AddIgnoredActor(Shooter);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);

	const FVector AimEnd = AimOrigin + AimDirection * ActiveDefinition->Range;
	FHitResult AimHit;
	const bool bAimHit = GetWorld()->LineTraceSingleByObjectType(
		AimHit, AimOrigin, AimEnd, Objects, Params);
	const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : AimEnd;
	FVector Direction = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = MuzzleTransform.GetUnitAxis(EAxis::X);
	}

	const FTransform SpawnTransform(Direction.Rotation(), MuzzleLocation);
	AFTProjectileActor* Projectile = GetWorld()->SpawnActorDeferred<AFTProjectileActor>(
		ActiveDefinition->ProjectileClass, SpawnTransform, ActiveItem, ShooterPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return false;
	}

	Projectile->InitializeProjectile(ActiveDefinition->Damage,
		GetAbilitySystemComponentFromActorInfo(), ActiveDefinition->EffectClass);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	return true;
}
