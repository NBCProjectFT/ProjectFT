#include "FTHitScanWeaponAction.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

bool UFTHitScanWeaponAction::ExecuteAction()
{
	AFTWeaponActor* WeaponActor = ActionComponent
		? Cast<AFTWeaponActor>(ActionComponent->GetOwner())
		: nullptr;
	if (!WeaponActor || !GetWorld())
	{
		return false;
	}

	const FTransform MuzzleTransform = WeaponActor->GetWeaponMuzzleTransform();
	AActor* Shooter = WeaponActor->GetOwner();
	if (!Shooter)
	{
		return false;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimOrigin = MuzzleLocation;
	FVector AimDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	AController* InstigatorController = nullptr;

	if (const APawn* ShooterPawn = Cast<APawn>(Shooter))
	{
		InstigatorController = ShooterPawn->GetController();
		if (const APlayerController* PlayerController = Cast<APlayerController>(InstigatorController))
		{
			FRotator AimRotation;
			PlayerController->GetPlayerViewPoint(AimOrigin, AimRotation);
			AimDirection = AimRotation.Vector();
		}
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FT_HitScan), true);
	QueryParams.AddIgnoredActor(WeaponActor);
	QueryParams.AddIgnoredActor(Shooter);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const FVector AimTraceEnd = AimOrigin + AimDirection * Definition.Range;
	FHitResult AimHit;
	const bool bAimHit = GetWorld()->LineTraceSingleByObjectType(
		AimHit, AimOrigin, AimTraceEnd, ObjectQueryParams, QueryParams);
	const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : AimTraceEnd;

	FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	}

	const FVector TraceEnd = MuzzleLocation + ShotDirection * Definition.Range;
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(
		Hit, MuzzleLocation, TraceEnd, ObjectQueryParams, QueryParams);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("HitScan aim: %s, muzzle hit: %s"),
		*GetNameSafe(bAimHit ? AimHit.GetActor() : nullptr), *GetNameSafe(HitActor));

	if (HitActor)
	{
		UGameplayStatics::ApplyDamage(
			HitActor, Definition.Damage,
			InstigatorController, WeaponActor, nullptr);
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(GetWorld(), AimOrigin, bAimHit ? AimHit.ImpactPoint : AimTraceEnd,
		bAimHit ? FColor::Blue : FColor::Cyan, false, 1.0f, 0, 1.0f);
	DrawDebugLine(GetWorld(), MuzzleLocation, bHit ? Hit.ImpactPoint : TraceEnd,
		bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.5f);
#endif

	return true;
}
