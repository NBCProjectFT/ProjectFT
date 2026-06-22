#include "FTWeaponGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

UFTWeaponGameplayAbility::UFTWeaponGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UFTWeaponGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	const AFTWeaponActor* Weapon = Spec ? Cast<AFTWeaponActor>(Spec->SourceObject.Get()) : nullptr;
	if (!Weapon || Weapon->GetOwner() != ActorInfo->AvatarActor.Get())
	{
		return false;
	}

	const FFTWeaponActionDefinition* Definition = nullptr;
	if (const UFTWeaponDataAsset* WeaponData = Weapon->GetWeaponDataAsset())
	{
		for (const FFTWeaponActionDefinition& Candidate : WeaponData->Actions)
		{
			if (Spec->GetDynamicSpecSourceTags().HasTagExact(Candidate.ActionTag))
			{
				Definition = &Candidate;
				break;
			}
		}
	}

	if (!Definition)
	{
		return false;
	}

	const UWorld* World = ActorInfo->AvatarActor.IsValid()
		? ActorInfo->AvatarActor->GetWorld()
		: nullptr;
	return World && World->GetTimeSeconds() >= LastExecutionTime + Definition->Cooldown;
}

void UFTWeaponGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ActiveWeapon = GetWeaponActor();
	ActiveDefinition = GetActionDefinition();
	if (!ActiveWeapon || !ActiveDefinition ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAbility(true);
		return;
	}

	PlayAttackMontage();
	if (!ExecuteWeaponAction())
	{
		FinishAbility(true);
		return;
	}

	LastExecutionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastExecutionTime;
	if (ShouldEndImmediately())
	{
		FinishAbility();
	}
}

void UFTWeaponGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ActiveWeapon = nullptr;
	ActiveDefinition = nullptr;
	PlayedMontageDuration = 0.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AFTWeaponActor* UFTWeaponGameplayAbility::GetWeaponActor() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	return Spec ? Cast<AFTWeaponActor>(Spec->SourceObject.Get()) : nullptr;
}

const FFTWeaponActionDefinition* UFTWeaponGameplayAbility::GetActionDefinition() const
{
	const AFTWeaponActor* Weapon = ActiveWeapon ? ActiveWeapon.Get() : GetWeaponActor();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	const UFTWeaponDataAsset* WeaponData = Weapon ? Weapon->GetWeaponDataAsset() : nullptr;
	if (!WeaponData || !Spec)
	{
		return nullptr;
	}

	for (const FFTWeaponActionDefinition& Definition : WeaponData->Actions)
	{
		if (Spec->GetDynamicSpecSourceTags().HasTagExact(Definition.ActionTag))
		{
			return &Definition;
		}
	}
	return nullptr;
}

bool UFTWeaponGameplayAbility::PlayAttackMontage()
{
	PlayedMontageDuration = 0.0f;
	const UFTItemDataAsset* ItemData = ActiveWeapon ? ActiveWeapon->ItemData : nullptr;
	if (!ItemData || ItemData->UseMontage.IsNull())
	{
		return false;
	}

	UAnimMontage* Montage = ItemData->UseMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = GetCurrentActorInfo() ? GetCurrentActorInfo()->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return false;
	}

	PlayedMontageDuration = AnimInstance->Montage_Play(Montage);
	return PlayedMontageDuration > 0.0f;
}

bool UFTWeaponGameplayAbility::ApplyWeaponDamage(AActor* TargetActor, float Damage) const
{
	if (!TargetActor || !ActiveWeapon || TargetActor == ActiveWeapon ||
		TargetActor == GetAvatarActorFromActorInfo())
	{
		return false;
	}
	if (!GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->HasAuthority())
	{
		return true;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (SourceASC && TargetASC && ActiveDefinition)
	{
		TSubclassOf<UGameplayEffect> EffectClass = ActiveDefinition->DamageEffectClass;
		if (!EffectClass)
		{
			EffectClass = UFTGE_Damage::StaticClass();
		}
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(ActiveWeapon);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
			EffectClass, GetAbilityLevel(), Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -Damage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			return true;
		}
	}

	AController* InstigatorController = nullptr;
	if (const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		InstigatorController = Pawn->GetController();
	}
	UGameplayStatics::ApplyDamage(
		TargetActor, Damage, InstigatorController, ActiveWeapon, nullptr);
	return true;
}

void UFTWeaponGameplayAbility::FinishAbility(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
		GetCurrentActivationInfo(), true, bWasCancelled);
}
