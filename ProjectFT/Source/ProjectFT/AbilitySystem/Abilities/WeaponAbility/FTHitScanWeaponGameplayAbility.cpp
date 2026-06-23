#include "FTHitScanWeaponGameplayAbility.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Item/FTItemActor.h"

bool UFTHitScanWeaponGameplayAbility::ExecuteWeaponAction()
{
	if (!ActiveItem || !ActiveDefinition || !GetWorld())
	{
		return false;
	}

	const FTransform MuzzleTransform = GetMuzzleTransform();
	AActor* Shooter = GetAvatarActorFromActorInfo();
	if (!Shooter)
	{
		return false;
	}
	if (!Shooter->HasAuthority())
	{
		return true;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimOrigin = MuzzleLocation;
	FVector AimDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	if (const APawn* Pawn = Cast<APawn>(Shooter))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			FRotator AimRotation;
			PC->GetPlayerViewPoint(AimOrigin, AimRotation);
			AimDirection = AimRotation.Vector();
		}
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FT_GAS_HitScan), true);
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
	FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = AimDirection.GetSafeNormal();
	}
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	}

	const FVector TraceEnd = MuzzleLocation + ShotDirection * ActiveDefinition->Range;
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(
		Hit, MuzzleLocation, TraceEnd, Objects, Params);
	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("HitScan hit actor: %s (%s)"),
			*GetNameSafe(Hit.GetActor()),
			Hit.GetActor() ? *Hit.GetActor()->GetClass()->GetName() : TEXT("None"));
		ApplyWeaponGameplayEffect(Hit.GetActor());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HitScan missed."));
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(GetWorld(), MuzzleLocation, bHit ? Hit.ImpactPoint : TraceEnd,
		bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.5f);
#endif
	return true;
}
