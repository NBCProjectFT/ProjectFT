#include "FTEquipmentComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Weapon/Ability/FTHitScanWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTMeleeWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTProjectileWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

UFTEquipmentComponent::UFTEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UFTEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureAbilitySystem();

	if (GetOwner() && GetOwner()->HasAuthority() && StartingWeaponClass)
	{
		if (!SpawnAndEquipWeapon(StartingWeaponClass))
		{
			UE_LOG(LogTemp, Error, TEXT("%s failed to spawn starting weapon %s."),
				*GetNameSafe(GetOwner()), *GetNameSafe(StartingWeaponClass));
		}
	}
	else if (GetOwner() && GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no StartingWeaponClass."), *GetNameSafe(GetOwner()));
	}
}

void UFTEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnequipWeapon();
	Super::EndPlay(EndPlayReason);
}

void UFTEquipmentComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, EquippedWeapon);
}

void UFTEquipmentComponent::StartListening()
{
	Super::StartListening();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Weapon_Action_Primary,
		this,
		&ThisClass::OnPrimaryAttackRequested));
}

void UFTEquipmentComponent::OnPrimaryAttackRequested(
	FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (Payload.InstigatorActor != GetOwner())
	{
		return;
	}

	AttackPrimary();
}

bool UFTEquipmentComponent::SpawnAndEquipWeapon(TSubclassOf<AFTWeaponActor> WeaponClass)
{
	AActor* EquipmentOwner = GetOwner();
	UWorld* World = GetWorld();
	if (!EquipmentOwner || !EquipmentOwner->HasAuthority() || !World || !WeaponClass)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = EquipmentOwner;
	SpawnParams.Instigator = Cast<APawn>(EquipmentOwner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFTWeaponActor* Weapon = World->SpawnActor<AFTWeaponActor>(
		WeaponClass, EquipmentOwner->GetActorTransform(), SpawnParams);
	if (!Weapon)
	{
		return false;
	}

	if (!EquipWeapon(Weapon))
	{
		Weapon->Destroy();
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("%s equipped %s."),
		*GetNameSafe(EquipmentOwner), *GetNameSafe(Weapon));
	return true;
}

bool UFTEquipmentComponent::EquipWeapon(AFTWeaponActor* Weapon)
{
	AActor* EquipmentOwner = GetOwner();
	USceneComponent* AttachTarget = ResolveAttachTarget();
	if (!EquipmentOwner || !AttachTarget || !Weapon)
	{
		return false;
	}

	if (EquippedWeapon == Weapon)
	{
		return true;
	}

	UnequipWeapon();
	const FName AttachSocketName = ResolveWeaponSocketName(AttachTarget);
	Weapon->SetOwner(EquipmentOwner);
	Weapon->SetInstigator(Cast<APawn>(EquipmentOwner));
	Weapon->SetActorEnableCollision(true);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Weapon);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		Primitive->SetSimulatePhysics(false);
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (!Weapon->AttachToComponent(
		AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName))
	{
		return false;
	}

	Weapon->SetActorRelativeTransform(WeaponRelativeTransform);
	EquippedWeapon = Weapon;
	if (!GrantWeaponAbilities(Weapon))
	{
		UE_LOG(LogTemp, Error, TEXT("%s failed to grant GAS abilities for %s."),
			*GetNameSafe(EquipmentOwner), *GetNameSafe(Weapon));
		EquippedWeapon = nullptr;
		Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Attached %s to %s socket '%s'."),
		*GetNameSafe(Weapon), *GetNameSafe(AttachTarget), *AttachSocketName.ToString());
	return true;
}

void UFTEquipmentComponent::UnequipWeapon()
{
	RemoveWeaponAbilities();
	if (!EquippedWeapon)
	{
		return;
	}

	AFTWeaponActor* Weapon = EquippedWeapon;
	EquippedWeapon = nullptr;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Weapon->Destroy();
	}
}

void UFTEquipmentComponent::AttackPrimary()
{
	if (EquippedWeapon && EquippedWeapon->GetOwner() == GetOwner())
	{
		TryActivateWeaponAbility(TAG_FT_Weapon_Action_Primary);
	}
}

bool UFTEquipmentComponent::TryActivateWeaponAbility(FGameplayTag ActionTag)
{
	if (!AbilitySystemComponent || !EquippedWeapon || !ActionTag.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpecHandle Handle = FindWeaponAbilityHandle(ActionTag);
	return Handle.IsValid() && AbilitySystemComponent->TryActivateAbility(Handle);
}

void UFTEquipmentComponent::NotifyWeaponActionWindowBegin(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowBegin();
	}
}

void UFTEquipmentComponent::NotifyWeaponActionWindowTick(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowTick();
	}
}

void UFTEquipmentComponent::NotifyWeaponActionWindowEnd(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowEnd();
	}
}

bool UFTEquipmentComponent::EnsureAbilitySystem()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	AbilitySystemComponent = OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!AbilitySystemComponent)
	{
		AbilitySystemComponent = NewObject<UAbilitySystemComponent>(
			OwnerActor, TEXT("WeaponAbilitySystemComponent"));
		OwnerActor->AddInstanceComponent(AbilitySystemComponent);
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning,
			TEXT("%s had no AbilitySystemComponent; created a temporary weapon ASC."),
			*GetNameSafe(OwnerActor));
	}

	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, OwnerActor);
	return true;
}

bool UFTEquipmentComponent::GrantWeaponAbilities(AFTWeaponActor* Weapon)
{
	RemoveWeaponAbilities();
	if (!Weapon || !EnsureAbilitySystem() || !GetOwner()->HasAuthority())
	{
		return Weapon && AbilitySystemComponent && !GetOwner()->HasAuthority();
	}

	const UFTWeaponDataAsset* WeaponData = Weapon->GetWeaponDataAsset();
	if (!WeaponData)
	{
		return false;
	}

	for (const FFTWeaponActionDefinition& Definition : WeaponData->Actions)
	{
		TSubclassOf<UFTWeaponGameplayAbility> AbilityClass = Definition.AbilityClass;
		if (!AbilityClass)
		{
			AbilityClass = ResolveDefaultAbilityClass(WeaponData);
		}
		if (!Definition.ActionTag.IsValid() || !AbilityClass ||
			GrantedWeaponAbilities.Contains(Definition.ActionTag))
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid GAS weapon action '%s' on %s."),
				*Definition.ActionTag.ToString(), *GetNameSafe(WeaponData));
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, Weapon);
		Spec.GetDynamicSpecSourceTags().AddTag(Definition.ActionTag);
		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
		GrantedWeaponAbilities.Add(Definition.ActionTag, Handle);
	}

	return !GrantedWeaponAbilities.IsEmpty();
}

void UFTEquipmentComponent::RemoveWeaponAbilities()
{
	if (AbilitySystemComponent && GetOwner() && GetOwner()->HasAuthority())
	{
		for (const TPair<FGameplayTag, FGameplayAbilitySpecHandle>& Pair : GrantedWeaponAbilities)
		{
			AbilitySystemComponent->CancelAbilityHandle(Pair.Value);
			AbilitySystemComponent->ClearAbility(Pair.Value);
		}
	}
	GrantedWeaponAbilities.Empty();
}

TSubclassOf<UFTWeaponGameplayAbility> UFTEquipmentComponent::ResolveDefaultAbilityClass(
	const UFTWeaponDataAsset* WeaponData) const
{
	if (!WeaponData)
	{
		return nullptr;
	}

	const bool bMelee = WeaponData->WeaponTags.HasTagExact(TAG_FT_Weapon_Type_Melee);
	const bool bHitScan = WeaponData->WeaponTags.HasTagExact(TAG_FT_Weapon_Type_HitScan);
	const bool bProjectile = WeaponData->WeaponTags.HasTagExact(TAG_FT_Weapon_Type_Projectile);
	if (static_cast<int32>(bMelee) + static_cast<int32>(bHitScan) +
		static_cast<int32>(bProjectile) != 1)
	{
		return nullptr;
	}

	if (bMelee)
	{
		return UFTMeleeWeaponGameplayAbility::StaticClass();
	}
	if (bHitScan)
	{
		return UFTHitScanWeaponGameplayAbility::StaticClass();
	}
	return UFTProjectileWeaponGameplayAbility::StaticClass();
}

FGameplayAbilitySpecHandle UFTEquipmentComponent::FindWeaponAbilityHandle(
	FGameplayTag ActionTag) const
{
	if (const FGameplayAbilitySpecHandle* Handle = GrantedWeaponAbilities.Find(ActionTag))
	{
		return *Handle;
	}

	if (!AbilitySystemComponent)
	{
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(ActionTag) &&
			(!EquippedWeapon || Spec.SourceObject.Get() == EquippedWeapon))
		{
			return Spec.Handle;
		}
	}
	return FGameplayAbilitySpecHandle();
}

UFTWeaponGameplayAbility* UFTEquipmentComponent::GetActiveWeaponAbility(
	FGameplayTag ActionTag) const
{
	if (!AbilitySystemComponent)
	{
		return nullptr;
	}

	const FGameplayAbilitySpecHandle Handle = FindWeaponAbilityHandle(ActionTag);
	FGameplayAbilitySpec* Spec = Handle.IsValid()
		? AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
		: nullptr;
	return Spec ? Cast<UFTWeaponGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
}

USceneComponent* UFTEquipmentComponent::ResolveAttachTarget() const
{
	AActor* EquipmentOwner = GetOwner();
	if (!EquipmentOwner)
	{
		return nullptr;
	}

	const FName SocketName = WeaponSocketName.IsNone() ? FName(TEXT("hand_r")) : WeaponSocketName;
	USkeletalMeshComponent* SkeletalMesh = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(EquipmentOwner))
	{
		SkeletalMesh = Character->GetMesh();
	}
	if (!SkeletalMesh)
	{
		SkeletalMesh = EquipmentOwner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (SkeletalMesh)
	{
		if (SkeletalMesh->DoesSocketExist(SocketName))
		{
			return SkeletalMesh;
		}

		UE_LOG(LogTemp, Warning, TEXT("%s has no weapon socket '%s'; using root component."),
			*GetNameSafe(EquipmentOwner), *SocketName.ToString());
	}

	return EquipmentOwner->GetRootComponent();
}

FName UFTEquipmentComponent::ResolveWeaponSocketName(USceneComponent* AttachTarget) const
{
	if (Cast<USkeletalMeshComponent>(AttachTarget))
	{
		return WeaponSocketName.IsNone() ? FName(TEXT("hand_r")) : WeaponSocketName;
	}

	return NAME_None;
}
