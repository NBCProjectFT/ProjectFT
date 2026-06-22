#include "FTMeleeWeaponGameplayAbility.h"

#include "Components/CapsuleComponent.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "TimerManager.h"

bool UFTMeleeWeaponGameplayAbility::ExecuteWeaponAction()
{
	if (!ActiveItem || !ActiveItem->GetMeleeHitComponent() || PlayedMontageDuration <= 0.0f)
	{
		return false;
	}

	GetWorld()->GetTimerManager().SetTimer(
		SafetyEndTimer, this, &ThisClass::HandleSafetyTimeout,
		PlayedMontageDuration + 0.1f, false);
	return true;
}

void UFTMeleeWeaponGameplayAbility::NotifyWindowBegin()
{
	if (!IsActive() || !ActiveItem)
	{
		return;
	}

	ActiveHitComponent = ActiveItem->GetMeleeHitComponent();
	if (!ActiveHitComponent)
	{
		return;
	}

	HitActors.Reset();
	ActiveHitComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActiveHitComponent->UpdateOverlaps();
	CheckMeleeHits();
}

void UFTMeleeWeaponGameplayAbility::NotifyWindowTick()
{
	CheckMeleeHits();
}

void UFTMeleeWeaponGameplayAbility::NotifyWindowEnd()
{
	if (ActiveHitComponent)
	{
		ActiveHitComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActiveHitComponent = nullptr;
	}
	FinishAbility();
}

void UFTMeleeWeaponGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActiveHitComponent)
	{
		ActiveHitComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActiveHitComponent = nullptr;
	}
	HitActors.Reset();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SafetyEndTimer);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTMeleeWeaponGameplayAbility::HandleSafetyTimeout()
{
	FinishAbility(true);
}

void UFTMeleeWeaponGameplayAbility::CheckMeleeHits()
{
	UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ActiveHitComponent);
	if (!Capsule || !ActiveDefinition)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	Capsule->GetOverlappingActors(OverlappingActors);
	for (AActor* OtherActor : OverlappingActors)
	{
		const TWeakObjectPtr<AActor> Key(OtherActor);
		if (!OtherActor || HitActors.Contains(Key))
		{
			continue;
		}
		if (ApplyWeaponDamage(OtherActor, ActiveDefinition->Damage))
		{
			HitActors.Add(Key);
		}
	}
}
