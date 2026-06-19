#include "FTHitScanWeaponAction.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "ProjectFT/Interface/FTWeaponSource.h"

bool UFTHitScanWeaponAction::ExecuteAction()
{
	AActor* WeaponActor = ActionComponent ? ActionComponent->GetOwner() : nullptr;
	if (!WeaponActor || !WeaponActor->Implements<UFTWeaponSource>() || !GetWorld())
	{
		return false;
	}

	IFTWeaponSource* WeaponSource = Cast<IFTWeaponSource>(WeaponActor);
	const FTransform MuzzleTransform = WeaponSource->GetWeaponMuzzleTransform();
	AActor* Shooter = WeaponActor->GetOwner();
	if (!Shooter)
	{
		return false;
	}

	const FVector TraceStart = MuzzleTransform.GetLocation();
	const FVector ShotDirection = MuzzleTransform.GetUnitAxis(EAxis::X);
	const FVector TraceEnd = TraceStart + ShotDirection * Definition.Range;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FT_HitScan), true);
	QueryParams.AddIgnoredActor(WeaponActor);
	QueryParams.AddIgnoredActor(Shooter);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	if (HitActor && HitActor->Implements<UFTDamageable>())
	{
		AController* InstigatorController = nullptr;
		if (const APawn* ShooterPawn = Cast<APawn>(Shooter))
		{
			InstigatorController = ShooterPawn->GetController();
		}

		UGameplayStatics::ApplyPointDamage(
			HitActor, Definition.Damage, ShotDirection, Hit,
			InstigatorController, WeaponActor, nullptr);
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : TraceEnd,
		bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.5f);
#endif

	return true;
}
