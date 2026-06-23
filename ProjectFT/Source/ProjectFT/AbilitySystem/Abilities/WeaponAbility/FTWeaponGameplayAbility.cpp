#include "FTWeaponGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
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

// bool UFTWeaponGameplayAbility::PrepareItemUse()
// {
// 	ActiveItem = GetItemActor();
// 	ActiveDefinition = GetActionDefinition();
// 	return ActiveItem && ActiveDefinition;
// }
//
// bool UFTWeaponGameplayAbility::ExecuteItemUse()
// {
// 	if (!ExecuteWeaponAction())
// 	{
// 		return false;
// 	}
// 	return true;
// }

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

FTransform UFTWeaponGameplayAbility::GetItemSocketTransform(FName SocketName) const
{
	const AFTItemActor* Item = ActiveItem ? ActiveItem.Get() : GetItemActor();
	const UStaticMeshComponent* MeshComponent = Item ? Item->GetItemMeshComponent() : nullptr;
	if (MeshComponent && MeshComponent->DoesSocketExist(SocketName))
	{
		return MeshComponent->GetSocketTransform(SocketName, RTS_World);
	}
	return Item ? Item->GetActorTransform() : FTransform::Identity;
}

FTransform UFTWeaponGameplayAbility::GetMuzzleTransform() const
{
	const AFTItemActor* Item = ActiveItem ? ActiveItem.Get() : GetItemActor();
	const UStaticMeshComponent* MeshComponent = Item ? Item->GetItemMeshComponent() : nullptr;
	const FName SocketName = Item && Item->ItemData ? Item->ItemData->MuzzleSocketName : NAME_None;
	if (MeshComponent && SocketName != NAME_None && MeshComponent->DoesSocketExist(SocketName))
	{
		return MeshComponent->GetSocketTransform(SocketName, RTS_World);
	}
	if (Item)
	{
		return Item->GetActorTransform();
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (AController* Controller = Pawn->GetController())
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
			return FTransform(ViewRotation, ViewLocation + ViewRotation.Vector() * 100.0f);
		}
	}

	return Avatar
		? FTransform(Avatar->GetActorRotation(),
			Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.0f)
		: GetItemSocketTransform(NAME_None);
}

bool UFTWeaponGameplayAbility::ApplyWeaponGameplayEffect(AActor* TargetActor) const
{
	if (!TargetActor || !ActiveItem || TargetActor == ActiveItem ||
		TargetActor == GetAvatarActorFromActorInfo())
	{
		return false;
	}
	if (!GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = ResolveAbilitySystemComponent(TargetActor);
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

	return false;
}

UAbilitySystemComponent* UFTWeaponGameplayAbility::ResolveAbilitySystemComponent(
	AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}
	if (UAbilitySystemComponent* AbilitySystemComponent =
		TargetActor->FindComponentByClass<UAbilitySystemComponent>())
	{
		return AbilitySystemComponent;
	}
	if (const IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(TargetActor))
	{
		return AbilitySystemInterface->GetAbilitySystemComponent();
	}
	return nullptr;
}
