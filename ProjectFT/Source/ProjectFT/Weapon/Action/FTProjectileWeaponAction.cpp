#include "FTProjectileWeaponAction.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"
#include "ProjectFT/Weapon/Projectile/FTProjectileActor.h"

bool UFTProjectileWeaponAction::ExecuteAction()
{
	AFTWeaponActor* WeaponActor = ActionComponent
		? Cast<AFTWeaponActor>(ActionComponent->GetOwner())
		: nullptr;
	UWorld* World = GetWorld();
	if (!WeaponActor || !World)
	{
		UE_LOG(LogTemp, Error, TEXT("Projectile attack failed: WeaponActor=%d World=%d"),
			WeaponActor != nullptr, World != nullptr);
		return false;
	}

	if (!Definition.ProjectileClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Projectile attack failed on '%s': ProjectileClass is not set in WeaponDataAsset."),
			*GetNameSafe(WeaponActor));
		return false;
	}

	AActor* Shooter = WeaponActor->GetOwner();
	if (!Shooter)
	{
		return false;
	}

	const FTransform MuzzleTransform = WeaponActor->GetWeaponMuzzleTransform();
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimOrigin = MuzzleLocation;
	FVector AimDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	APawn* ShooterPawn = Cast<APawn>(Shooter);

	if (const APlayerController* PlayerController = ShooterPawn
		? Cast<APlayerController>(ShooterPawn->GetController())
		: nullptr)
	{
		FRotator AimRotation;
		PlayerController->GetPlayerViewPoint(AimOrigin, AimRotation);
		AimDirection = AimRotation.Vector();
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FT_ProjectileAim), true);
	QueryParams.AddIgnoredActor(WeaponActor);
	QueryParams.AddIgnoredActor(Shooter);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const FVector AimTraceEnd = AimOrigin + AimDirection * Definition.Range;
	FHitResult AimHit;
	const bool bAimHit = World->LineTraceSingleByObjectType(
		AimHit, AimOrigin, AimTraceEnd, ObjectQueryParams, QueryParams);
	const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : AimTraceEnd;

	FVector SpawnDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (SpawnDirection.IsNearlyZero())
	{
		SpawnDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	}

	const FTransform SpawnTransform(SpawnDirection.Rotation(), MuzzleLocation);
	AFTProjectileActor* Projectile = World->SpawnActorDeferred<AFTProjectileActor>(
		Definition.ProjectileClass,
		SpawnTransform,
		WeaponActor,
		ShooterPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Error, TEXT("Projectile attack failed on '%s': deferred spawn failed."),
			*GetNameSafe(WeaponActor));
		return false;
	}

	Projectile->InitializeProjectile(Definition.Damage);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	UE_LOG(LogTemp, Log, TEXT("Spawned projectile '%s' from '%s'."),
		*GetNameSafe(Projectile), *GetNameSafe(WeaponActor));
	return true;
}
