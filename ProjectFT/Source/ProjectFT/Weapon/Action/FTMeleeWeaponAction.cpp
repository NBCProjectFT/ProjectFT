#include "FTMeleeWeaponAction.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

bool UFTMeleeWeaponAction::ExecuteAction()
{
	AFTWeaponActor* WeaponActor = ActionComponent
		? Cast<AFTWeaponActor>(ActionComponent->GetOwner())
		: nullptr;
	return WeaponActor && WeaponActor->GetMeleeHitComponent();
}

void UFTMeleeWeaponAction::NotifyWindowBegin()
{
	AFTWeaponActor* WeaponActor = ActionComponent
		? Cast<AFTWeaponActor>(ActionComponent->GetOwner())
		: nullptr;
	ActiveHitComponent = WeaponActor ? WeaponActor->GetMeleeHitComponent() : nullptr;
	if (!ActiveHitComponent)
	{
		return;
	}

	HitActors.Reset();
	ActiveHitComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActiveHitComponent->UpdateOverlaps();
	CheckMeleeHits();
}

void UFTMeleeWeaponAction::NotifyWindowTick()
{
	CheckMeleeHits();
}

void UFTMeleeWeaponAction::NotifyWindowEnd()
{
	if (ActiveHitComponent)
	{
		ActiveHitComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActiveHitComponent = nullptr;
	}
}

void UFTMeleeWeaponAction::CheckMeleeHits()
{
	UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ActiveHitComponent);
	if (!Capsule)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	Capsule->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ProcessOverlappingActor(OverlappingActor);
	}
}

void UFTMeleeWeaponAction::ProcessOverlappingActor(AActor* OtherActor)
{
	AActor* WeaponActor = ActionComponent ? ActionComponent->GetOwner() : nullptr;
	AActor* Attacker = WeaponActor ? WeaponActor->GetOwner() : nullptr;
	if (!OtherActor || OtherActor == WeaponActor || OtherActor == Attacker)
	{
		return;
	}

	const TWeakObjectPtr<AActor> HitActorKey(OtherActor);
	if (HitActors.Contains(HitActorKey))
	{
		return;
	}

	HitActors.Add(HitActorKey);
	AController* InstigatorController = nullptr;
	if (const APawn* AttackerPawn = Cast<APawn>(Attacker))
	{
		InstigatorController = AttackerPawn->GetController();
	}

	UGameplayStatics::ApplyDamage(
		OtherActor, Definition.Damage, InstigatorController, WeaponActor, nullptr);
}
