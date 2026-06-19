#include "FTEquipmentComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

UFTEquipmentComponent::UFTEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (StartingWeaponClass)
	{
		if (!SpawnAndEquipWeapon(StartingWeaponClass))
		{
			UE_LOG(LogTemp, Error, TEXT("%s failed to spawn starting weapon %s."),
				*GetNameSafe(GetOwner()), *GetNameSafe(StartingWeaponClass));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no StartingWeaponClass."), *GetNameSafe(GetOwner()));
	}
}

void UFTEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnequipWeapon();
	Super::EndPlay(EndPlayReason);
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
	if (!EquipmentOwner || !World || !WeaponClass)
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
	UE_LOG(LogTemp, Log, TEXT("Attached %s to %s socket '%s'."),
		*GetNameSafe(Weapon), *GetNameSafe(AttachTarget), *AttachSocketName.ToString());
	return true;
}

void UFTEquipmentComponent::UnequipWeapon()
{
	if (!EquippedWeapon)
	{
		return;
	}

	AFTWeaponActor* Weapon = EquippedWeapon;
	EquippedWeapon = nullptr;
	Weapon->Destroy();
}

void UFTEquipmentComponent::AttackPrimary()
{
	if (EquippedWeapon && EquippedWeapon->GetOwner() == GetOwner())
	{
		EquippedWeapon->Attack();
	}
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
