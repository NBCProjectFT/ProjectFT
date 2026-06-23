#include "FTMeleeWeaponGameplayAbility.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "TimerManager.h"

bool UFTMeleeWeaponGameplayAbility::ExecuteWeaponAction()
{
	if (!ActiveItem || PlayedMontageDuration <= 0.0f)
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

	HitActors.Reset();
	CheckMeleeHits();
}

void UFTMeleeWeaponGameplayAbility::NotifyWindowTick()
{
	CheckMeleeHits();
}

void UFTMeleeWeaponGameplayAbility::NotifyWindowEnd()
{
	FinishAbility();
}

void UFTMeleeWeaponGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
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
	if (!ActiveItem || !ActiveDefinition || !GetWorld())
	{
		return;
	}

	FVector TraceStart;
	FVector TraceEnd;
	float TraceRadius = 0.0f;
	if (!ResolveMeleeTraceSegment(TraceStart, TraceEnd, TraceRadius))
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FT_GAS_MeleeSocketSweep), false);
	Params.AddIgnoredActor(ActiveItem);
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Params.AddIgnoredActor(Avatar);
	}

	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(
		Hits,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		Objects,
		FCollisionShape::MakeSphere(TraceRadius),
		Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* OtherActor = Hit.GetActor();
		const TWeakObjectPtr<AActor> Key(OtherActor);
		if (!OtherActor || HitActors.Contains(Key))
		{
			continue;
		}
		if (ApplyWeaponGameplayEffect(OtherActor))
		{
			HitActors.Add(Key);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (ActiveItem->ItemData && ActiveItem->ItemData->bDrawMeleeTraceDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.1f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), TraceStart, TraceRadius, 12, FColor::Red, false, 0.1f);
		DrawDebugSphere(GetWorld(), TraceEnd, TraceRadius, 12, FColor::Red, false, 0.1f);
	}
#endif
}

bool UFTMeleeWeaponGameplayAbility::ResolveMeleeTraceSegment(
	FVector& OutStart, FVector& OutEnd, float& OutRadius) const
{
	if (!ActiveItem || !ActiveItem->ItemData)
	{
		return false;
	}

	const UFTItemDataAsset* ItemData = ActiveItem->ItemData;
	OutRadius = FMath::Max(1.0f, ItemData->MeleeTraceRadius);

	const UStaticMeshComponent* MeshComponent = ActiveItem->GetItemMeshComponent();
	if (MeshComponent &&
		MeshComponent->DoesSocketExist(ItemData->MeleeTraceStartSocketName) &&
		MeshComponent->DoesSocketExist(ItemData->MeleeTraceEndSocketName))
	{
		OutStart = MeshComponent->GetSocketLocation(ItemData->MeleeTraceStartSocketName);
		OutEnd = MeshComponent->GetSocketLocation(ItemData->MeleeTraceEndSocketName);
		return true;
	}

	return false;
}
