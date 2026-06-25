// Fill out your copyright notice in the Description page of Project Settings.


#include "FTGA_ProjectileAttack.h"

#include "ProjectFT/Data/FTProjectileDataAsset.h"

UFTGA_ProjectileAttack::UFTGA_ProjectileAttack()
{
}

void UFTGA_ProjectileAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UFTGA_ProjectileAttack::EndProjectileAbility(bool bWasCancelled)
{
}

const FFTProjectileAttackStruct* UFTGA_ProjectileAttack::GetProjectileAttackData() const
{
	return ProjectileDataAsset ? &ProjectileDataAsset->ProjectileAttackData : nullptr;
}
