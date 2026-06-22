#include "FTWeaponGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Message/FTGameplayTags.h"

UFTWeaponGameplayAbility::UFTWeaponGameplayAbility()
{
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
	const AFTItemActor* Item = Spec ? Cast<AFTItemActor>(Spec->SourceObject.Get()) : nullptr;
	if (!Item || Item->GetOwner() != ActorInfo->AvatarActor.Get())
	{
		return false;
	}

	const UFTWeaponDataAsset* WeaponData = Item->GetWeaponDataAsset();
	if (!WeaponData || !Spec)
	{
		return false;
	}

	const FFTWeaponActionDefinition* Definition = WeaponData->Actions.FindByPredicate(
		[Spec](const FFTWeaponActionDefinition& Candidate)
		{
			return Spec->GetDynamicSpecSourceTags().HasTagExact(Candidate.ActionTag);
		});
	if (!Definition)
	{
		return false;
	}

	const UWorld* World = ActorInfo->AvatarActor.IsValid()
		? ActorInfo->AvatarActor->GetWorld()
		: nullptr;
	return World && World->GetTimeSeconds() >= LastExecutionTime + Definition->Cooldown;
}

bool UFTWeaponGameplayAbility::PrepareItemUse()
{
	ActiveItem = GetItemActor();
	ActiveDefinition = GetActionDefinition();
	return ActiveItem && ActiveDefinition;
}

bool UFTWeaponGameplayAbility::ExecuteItemUse()
{
	if (!ExecuteWeaponAction())
	{
		return false;
	}
	LastExecutionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastExecutionTime;
	return true;
}

void UFTWeaponGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ActiveItem = nullptr;
	ActiveDefinition = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo,
		bReplicateEndAbility, bWasCancelled);
}

AFTItemActor* UFTWeaponGameplayAbility::GetItemActor() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	return Spec ? Cast<AFTItemActor>(Spec->SourceObject.Get()) : nullptr;
}

const FFTWeaponActionDefinition* UFTWeaponGameplayAbility::GetActionDefinition() const
{
	const AFTItemActor* Item = ActiveItem ? ActiveItem.Get() : GetItemActor();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	const UFTWeaponDataAsset* WeaponData = Item ? Item->GetWeaponDataAsset() : nullptr;
	if (!WeaponData || !Spec)
	{
		return nullptr;
	}

	return WeaponData->Actions.FindByPredicate(
		[Spec](const FFTWeaponActionDefinition& Definition)
		{
			return Spec->GetDynamicSpecSourceTags().HasTagExact(Definition.ActionTag);
		});
}

bool UFTWeaponGameplayAbility::ApplyWeaponDamage(AActor* TargetActor, float Damage) const
{
	if (!TargetActor || !ActiveItem || TargetActor == ActiveItem ||
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
		Context.AddSourceObject(ActiveItem);
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
		TargetActor, Damage, InstigatorController, ActiveItem, nullptr);
	return true;
}
