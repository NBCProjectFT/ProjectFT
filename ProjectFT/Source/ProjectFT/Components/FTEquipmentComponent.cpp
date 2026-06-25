// Fill out your copyright notice in the Description page of Project Settings.

#include "FTEquipmentComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

UFTEquipmentComponent::UFTEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	ItemActorClass = AFTItemActor::StaticClass();
}

void UFTEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bEquipDefaultOnBeginPlay && DefaultItemData)
	{
		EquipItem(DefaultItemData);
	}

	if (bGrantDefaultItemAbilityOnBeginPlay && DefaultItemData)
	{
		EnsureAbilityGranted(ResolveOwnerAbilitySystem(), DefaultItemData);
	}
}

void UFTEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableTemporaryAttackInput || !IsLocalPlayerOwner())
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	const APlayerController* PlayerController = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (PlayerController && PlayerController->WasInputKeyJustPressed(TemporaryAttackKey))
	{
		TryAttack();
	}
}

void UFTEquipmentComponent::EquipItem(UFTItemDataAsset* ItemData)
{
	if (!ItemData)
	{
		UnequipCurrentItem();
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	USkeletalMeshComponent* OwnerMesh = ResolveOwnerMesh();
	if (!Owner || !World || !OwnerMesh || !ItemActorClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("EquipItem failed. Owner=%s World=%s Mesh=%s ItemActorClass=%s"),
			*GetNameSafe(Owner),
			World ? TEXT("Valid") : TEXT("Missing"),
			*GetNameSafe(OwnerMesh),
			*GetNameSafe(ItemActorClass));
		return;
	}

	UnequipCurrentItem();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Owner->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedItemActor = World->SpawnActor<AFTItemActor>(ItemActorClass, Owner->GetActorTransform(), SpawnParams);
	if (!EquippedItemActor)
	{
		return;
	}

	EquippedItemActor->ItemData = ItemData;
	EquippedItemActor->UpdateAppearance();
	EquippedItemActor->SetActorEnableCollision(false);

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	EquippedItemActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetGenerateOverlapEvents(false);
	}

	const FName AttachSocketName = ResolveAttachSocketName(ItemData);
	if (!AttachSocketName.IsNone() && !OwnerMesh->DoesSocketExist(AttachSocketName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("Equip socket '%s' does not exist on mesh '%s'. Item will attach to mesh root."),
			*AttachSocketName.ToString(),
			*GetNameSafe(OwnerMesh));
	}

	EquippedItemActor->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		OwnerMesh->DoesSocketExist(AttachSocketName) ? AttachSocketName : NAME_None);

	EquippedItemActor->SetActorRelativeLocation(RelativeLocation);
	EquippedItemActor->SetActorRelativeRotation(RelativeRotation);
	EquippedItemActor->SetActorRelativeScale3D(RelativeScale);

	UE_LOG(LogFTItem, Log, TEXT("Equipped item. Owner=%s Item=%s Socket=%s Actor=%s"),
		*GetNameSafe(Owner),
		*GetNameSafe(ItemData),
		*AttachSocketName.ToString(),
		*GetNameSafe(EquippedItemActor));
}

void UFTEquipmentComponent::UnequipCurrentItem()
{
	if (EquippedItemActor)
	{
		EquippedItemActor->Destroy();
		EquippedItemActor = nullptr;
	}
}

bool UFTEquipmentComponent::TryAttack()
{
	UFTItemDataAsset* ItemData = DefaultItemData;
	if (!ItemData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: DefaultItemData is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	const TSubclassOf<UFTGameplayAbility> UseAbility = ItemData->ItemData.UseData.UseAbility;
	if (!UseAbility)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: UseAbility is missing. Item=%s"),
			*GetNameSafe(ItemData));
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = ResolveOwnerAbilitySystem();
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: ASC is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	EnsureAbilityGranted(AbilitySystemComponent, ItemData);

	const UFTGameplayAbility* AbilityCDO = UseAbility.GetDefaultObject();
	const FGameplayTag EventTag = AbilityCDO ? AbilityCDO->GetTriggerEventTag() : FGameplayTag();
	if (!EventTag.IsValid())
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: Ability has no trigger tag. Ability=%s Item=%s"),
			*GetNameSafe(AbilityCDO),
			*GetNameSafe(ItemData));
		return false;
	}

	const FTItemUseStruct& UseData = ItemData->ItemData.UseData;
	if (UseData.CooldownSeconds > 0.0f
		&& AbilitySystemComponent->HasMatchingGameplayTag(UFTGA_ItemAbility::ResolveCooldownTag(UseData)))
	{
		UE_LOG(LogFTItem, Verbose, TEXT("Temporary attack blocked by cooldown. Owner=%s Item=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ItemData));
		return false;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = GetOwner();
	Payload.Target = GetOwner();
	Payload.OptionalObject = ItemData;

	const int32 ActivatedCount = AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
	UE_LOG(LogFTItem, Log, TEXT("Temporary attack requested. Owner=%s Item=%s EventTag=%s ActivatedCount=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ItemData),
		*EventTag.ToString(),
		ActivatedCount);

	return ActivatedCount > 0;
}

FName UFTEquipmentComponent::ResolveAttachSocketName(const UFTItemDataAsset* ItemData) const
{
	if (const UFTMeleeDataAsset* MeleeDataAsset = Cast<UFTMeleeDataAsset>(ItemData))
	{
		if (!MeleeDataAsset->MeleeAttackData.AttachSocketName.IsNone())
		{
			return MeleeDataAsset->MeleeAttackData.AttachSocketName;
		}
	}

	return FallbackAttachSocketName;
}

USkeletalMeshComponent* UFTEquipmentComponent::ResolveOwnerMesh() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}

	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

UAbilitySystemComponent* UFTEquipmentComponent::ResolveOwnerAbilitySystem() const
{
	return GetOwner() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()) : nullptr;
}

bool UFTEquipmentComponent::EnsureAbilityGranted(UAbilitySystemComponent* AbilitySystemComponent, UFTItemDataAsset* ItemData) const
{
	if (!AbilitySystemComponent || !ItemData || !ItemData->ItemData.UseData.UseAbility)
	{
		return false;
	}

	const TSubclassOf<UFTGameplayAbility> UseAbility = ItemData->ItemData.UseData.UseAbility;
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetClass() == UseAbility)
		{
			return true;
		}
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UseAbility));
	UE_LOG(LogFTItem, Log, TEXT("Temporary attack granted ability. Owner=%s Item=%s Ability=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ItemData),
		*GetNameSafe(UseAbility));
	return true;
}

bool UFTEquipmentComponent::IsLocalPlayerOwner() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter && OwnerCharacter->IsLocallyControlled();
}
