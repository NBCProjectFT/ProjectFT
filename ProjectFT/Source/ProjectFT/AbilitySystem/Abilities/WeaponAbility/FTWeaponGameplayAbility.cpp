#include "FTWeaponGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

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

	if (!Item->ItemData || !Spec)
	{
		return false;
	}

	const FFTItemActionDefinition* Definition = Item->ItemData->Actions.FindByPredicate(
		[Spec](const FFTItemActionDefinition& Candidate)
		{
			const FGameplayTag ActionTag = Candidate.ActionTag.IsValid()
				? Candidate.ActionTag
				: TAG_FT_Weapon_Action_Primary;
			return Spec->GetDynamicSpecSourceTags().HasTagExact(ActionTag);
		});
	return Definition != nullptr;
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

const FFTItemActionDefinition* UFTWeaponGameplayAbility::GetActionDefinition() const
{
	const AFTItemActor* Item = ActiveItem ? ActiveItem.Get() : GetItemActor();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	if (!Item || !Item->ItemData || !Spec)
	{
		return nullptr;
	}

	return Item->ItemData->Actions.FindByPredicate(
		[Spec](const FFTItemActionDefinition& Definition)
		{
			const FGameplayTag ActionTag = Definition.ActionTag.IsValid()
				? Definition.ActionTag
				: TAG_FT_Weapon_Action_Primary;
			return Spec->GetDynamicSpecSourceTags().HasTagExact(ActionTag);
		});
}

bool UFTWeaponGameplayAbility::ApplyWeaponDamage(AActor* TargetActor) const
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
	if (!TargetASC)
	{
		if (const IAbilitySystemInterface* AbilitySystemInterface =
			Cast<IAbilitySystemInterface>(TargetActor))
		{
			TargetASC = AbilitySystemInterface->GetAbilitySystemComponent();
		}
	}
	if (SourceASC && TargetASC && ActiveDefinition)
	{
		TSubclassOf<UGameplayEffect> EffectClass = ActiveDefinition->EffectClass;
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
			if (SetByCallerDamage > 0.0f)
			{
				Spec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -SetByCallerDamage);
			}
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
		TargetActor, SetByCallerDamage, InstigatorController, ActiveItem, nullptr);
	return true;
}
