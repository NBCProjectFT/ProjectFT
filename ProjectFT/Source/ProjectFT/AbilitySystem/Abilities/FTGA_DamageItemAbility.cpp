#include "FTGA_DamageItemAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Interface/FTDamageable.h"

float UFTGA_DamageItemAbility::ResolveActiveDamageAmount() const
{
	const float* DamageMagnitude = ActiveUseData.EffectMagnitudes.Find(TAG_FT_Data_Damage_Object);
	if (!DamageMagnitude || *DamageMagnitude >= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Abs(*DamageMagnitude);
}

bool UFTGA_DamageItemAbility::ApplyDamageToDamageableTarget(AActor* TargetActor, const FHitResult* HitResult) const
{
	if (!TargetActor || !TargetActor->Implements<UFTDamageable>())
	{
		return false;
	}

	// ASC 대상은 GEBP_MeleeDamage/GameplayEffect 경로에서 이미 처리된다.
	if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		return false;
	}

	const float DamageAmount = ResolveActiveDamageAmount();
	if (DamageAmount <= 0.0f)
	{
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AController* InstigatorController = AvatarActor ? AvatarActor->GetInstigatorController() : nullptr;
	const FVector HitDirection = AvatarActor ? AvatarActor->GetActorForwardVector() : FVector::ZeroVector;
	FHitResult DamageHit = HitResult ? *HitResult : FHitResult();
	if (DamageHit.Location.IsNearlyZero() && DamageHit.ImpactPoint.IsNearlyZero())
	{
		DamageHit.Location = TargetActor->GetActorLocation();
		DamageHit.ImpactPoint = TargetActor->GetActorLocation();
	}

	UGameplayStatics::ApplyPointDamage(
		TargetActor,
		DamageAmount,
		HitDirection,
		DamageHit,
		InstigatorController,
		AvatarActor,
		UDamageType::StaticClass()
	);

	return true;
}
