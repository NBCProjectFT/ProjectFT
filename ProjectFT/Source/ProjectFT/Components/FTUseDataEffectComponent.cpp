#include "FTUseDataEffectComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"
#include "ProjectFT/AbilitySystem/FTUseDataEffectLibrary.h"

UFTUseDataEffectComponent::UFTUseDataEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CooldownEffectClass = UFTGE_Cooldown::StaticClass();
}

bool UFTUseDataEffectComponent::TryExecuteUseData(AActor* TargetActor)
{
	if (bCasting)
	{
		Fail(TEXT("AlreadyCasting"));
		return false;
	}

	if (!IsValid(TargetActor) || !GetTargetASC(TargetActor))
	{
		Fail(TEXT("InvalidTarget"));
		return false;
	}

	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!CanStartUse(OwnerASC))
	{
		Fail(TEXT("Cooldown"));
		return false;
	}

	PendingTargetActor = TargetActor;
	if (UseData.CastTimeSeconds > 0.0f)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			Fail(TEXT("NoWorld"));
			PendingTargetActor.Reset();
			return false;
		}

		bCasting = true;
		World->GetTimerManager().SetTimer(
			CastTimerHandle,
			this,
			&ThisClass::FinishExecuteUseData,
			UseData.CastTimeSeconds,
			false);
		return true;
	}

	FinishExecuteUseData();
	return true;
}

void UFTUseDataEffectComponent::CancelPendingUse()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CastTimerHandle);
	}

	bCasting = false;
	PendingTargetActor.Reset();
}

bool UFTUseDataEffectComponent::IsOnCooldown() const
{
	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (OwnerASC)
	{
		return UFTUseDataEffectLibrary::IsUseDataOnCooldown(OwnerASC, UseData);
	}

	if (!bUseLocalCooldownWhenOwnerHasNoASC || UseData.CooldownSeconds <= 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < LocalCooldownEndTime;
}

void UFTUseDataEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelPendingUse();
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UFTUseDataEffectComponent::GetOwnerASC() const
{
	return GetOwner()
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner())
		: nullptr;
}

UAbilitySystemComponent* UFTUseDataEffectComponent::GetTargetASC(AActor* TargetActor) const
{
	return TargetActor
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor)
		: nullptr;
}

bool UFTUseDataEffectComponent::CanStartUse(UAbilitySystemComponent* OwnerASC) const
{
	if (OwnerASC)
	{
		return !UFTUseDataEffectLibrary::IsUseDataOnCooldown(OwnerASC, UseData);
	}

	if (!bUseLocalCooldownWhenOwnerHasNoASC || UseData.CooldownSeconds <= 0.0f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	return !World || World->GetTimeSeconds() >= LocalCooldownEndTime;
}

void UFTUseDataEffectComponent::FinishExecuteUseData()
{
	bCasting = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CastTimerHandle);
	}

	AActor* TargetActor = PendingTargetActor.Get();
	PendingTargetActor.Reset();

	UAbilitySystemComponent* TargetASC = GetTargetASC(TargetActor);
	if (!TargetASC)
	{
		Fail(TEXT("InvalidTarget"));
		return;
	}

	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!CanStartUse(OwnerASC))
	{
		Fail(TEXT("Cooldown"));
		return;
	}

	const int32 AppliedCount = UFTUseDataEffectLibrary::ApplyUseEffectsFromASC(
		OwnerASC,
		TargetASC,
		UseData);
	if (AppliedCount <= 0)
	{
		Fail(TEXT("NoEffectApplied"));
		return;
	}

	ApplyCooldown(OwnerASC);
	OnExecuted.Broadcast(AppliedCount);
}

void UFTUseDataEffectComponent::ApplyCooldown(UAbilitySystemComponent* OwnerASC)
{
	if (UseData.CooldownSeconds <= 0.0f)
	{
		return;
	}

	if (OwnerASC)
	{
		UFTUseDataEffectLibrary::ApplyCooldownFromASC(OwnerASC, UseData, CooldownEffectClass);
		return;
	}

	if (bUseLocalCooldownWhenOwnerHasNoASC)
	{
		if (const UWorld* World = GetWorld())
		{
			LocalCooldownEndTime = World->GetTimeSeconds() + UseData.CooldownSeconds;
		}
	}
}

void UFTUseDataEffectComponent::Fail(const FName Reason)
{
	OnFailed.Broadcast(Reason);
}
