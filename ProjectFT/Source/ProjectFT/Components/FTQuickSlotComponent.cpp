#include "FTQuickSlotComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Weapon/Ability/FTHitScanWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTMeleeWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTProjectileWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/Ability/FTWeaponGameplayAbility.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

UFTQuickSlotComponent::UFTQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Slots.SetNum(4);
}

void UFTQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();
	Slots.SetNum(4);
	AbilitySystemComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UAbilitySystemComponent>()
		: nullptr;
}

void UFTQuickSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnequipCurrentItem();
	Super::EndPlay(EndPlayReason);
}

void UFTQuickSlotComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
	DOREPLIFETIME(ThisClass, EquippedItemActor);
}

bool UFTQuickSlotComponent::AssignItemToSlot(
	int32 SlotIndex, UFTItemDataAsset* ItemData, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || !ItemData || Quantity <= 0)
	{
		return false;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerAssignItemToSlot(SlotIndex, ItemData, Quantity);
		return true;
	}

	Slots[SlotIndex].ItemData = ItemData;
	Slots[SlotIndex].Quantity = Quantity;
	if (ActiveSlotIndex == SlotIndex)
	{
		EquipSelectedItem();
	}
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UFTQuickSlotComponent::ClearSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerClearSlot(SlotIndex);
		return true;
	}

	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipCurrentItem();
		ActiveSlotIndex = INDEX_NONE;
		OnActiveQuickSlotChanged.Broadcast();
	}
	Slots[SlotIndex] = FFTQuickSlotEntry();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UFTQuickSlotComponent::SelectSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSelectSlot(SlotIndex);
		return true;
	}

	UnequipCurrentItem();
	ActiveSlotIndex = SlotIndex;
	const bool bEquipped = Slots[SlotIndex].IsEmpty() || EquipSelectedItem();
	OnActiveQuickSlotChanged.Broadcast();
	return bEquipped;
}

bool UFTQuickSlotComponent::SelectSlotByInputTag(FGameplayTag InputTag)
{
	const int32 SlotIndex = ResolveSlotIndex(InputTag);
	return SlotIndex != INDEX_NONE && SelectSlot(SlotIndex);
}

bool UFTQuickSlotComponent::HandleInputTag(FGameplayTag InputTag)
{
	if (InputTag.MatchesTagExact(TAG_FT_Input_Item_Primary))
	{
		return UseSelectedItem();
	}
	return SelectSlotByInputTag(InputTag);
}

bool UFTQuickSlotComponent::UseSelectedItem()
{
	UFTItemDataAsset* ItemData = GetActiveItemData();
	if (!AbilitySystemComponent || !ItemData)
	{
		return false;
	}

	const FGameplayTag ActionTag = ItemData->WeaponDataAsset
		? TAG_FT_Weapon_Action_Primary
		: ItemData->PrimaryUseTag;
	const FGameplayAbilitySpecHandle Handle = FindSelectedAbilityHandle(ActionTag);
	return Handle.IsValid() && AbilitySystemComponent->TryActivateAbility(Handle);
}

void UFTQuickSlotComponent::NotifyWeaponActionWindowBegin(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowBegin();
	}
}

void UFTQuickSlotComponent::NotifyWeaponActionWindowTick(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowTick();
	}
}

void UFTQuickSlotComponent::NotifyWeaponActionWindowEnd(FGameplayTag ActionTag)
{
	if (UFTWeaponGameplayAbility* Ability = GetActiveWeaponAbility(ActionTag))
	{
		Ability->NotifyWindowEnd();
	}
}

bool UFTQuickSlotComponent::ConsumeSelectedItem(int32 Amount)
{
	if (Amount <= 0 || !Slots.IsValidIndex(ActiveSlotIndex) ||
		Slots[ActiveSlotIndex].IsEmpty())
	{
		return false;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerConsumeSelectedItem(Amount);
		return true;
	}

	FFTQuickSlotEntry& Entry = Slots[ActiveSlotIndex];
	Entry.Quantity = FMath::Max(0, Entry.Quantity - Amount);
	if (Entry.Quantity == 0)
	{
		const int32 EmptySlot = ActiveSlotIndex;
		ClearSlot(EmptySlot);
		return true;
	}

	OnQuickSlotsChanged.Broadcast();
	return true;
}

UFTItemDataAsset* UFTQuickSlotComponent::GetActiveItemData() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) && !Slots[ActiveSlotIndex].IsEmpty()
		? Slots[ActiveSlotIndex].ItemData.Get()
		: nullptr;
}

void UFTQuickSlotComponent::ServerAssignItemToSlot_Implementation(
	int32 SlotIndex, UFTItemDataAsset* ItemData, int32 Quantity)
{
	AssignItemToSlot(SlotIndex, ItemData, Quantity);
}

void UFTQuickSlotComponent::ServerClearSlot_Implementation(int32 SlotIndex)
{
	ClearSlot(SlotIndex);
}

void UFTQuickSlotComponent::ServerSelectSlot_Implementation(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
}

void UFTQuickSlotComponent::ServerConsumeSelectedItem_Implementation(int32 Amount)
{
	ConsumeSelectedItem(Amount);
}

void UFTQuickSlotComponent::OnRep_Slots()
{
	OnQuickSlotsChanged.Broadcast();
}

void UFTQuickSlotComponent::OnRep_ActiveSlotIndex()
{
	OnActiveQuickSlotChanged.Broadcast();
}

bool UFTQuickSlotComponent::EquipSelectedItem()
{
	UFTItemDataAsset* ItemData = GetActiveItemData();
	if (!ItemData || !GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
	{
		return false;
	}

	RemoveSelectedItemAbilities();
	if (EquippedItemActor)
	{
		EquippedItemActor->Destroy();
		EquippedItemActor = nullptr;
	}

	TSubclassOf<AFTItemActor> ActorClass = ItemData->EquippedActorClass;
	if (!ActorClass)
	{
		ActorClass = AFTItemActor::StaticClass();
	}
	AFTItemActor* ItemActor = GetWorld()->SpawnActorDeferred<AFTItemActor>(
		ActorClass, GetOwner()->GetActorTransform(), GetOwner(),
		Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!ItemActor)
	{
		return false;
	}

	ItemActor->ItemData = ItemData;
	if (AFTWeaponActor* WeaponActor = Cast<AFTWeaponActor>(ItemActor))
	{
		WeaponActor->SetWeaponDataAsset(ItemData->WeaponDataAsset);
	}
	UGameplayStatics::FinishSpawningActor(ItemActor, GetOwner()->GetActorTransform());
	TInlineComponentArray<UPrimitiveComponent*> Primitives(ItemActor);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetSimulatePhysics(false);
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	USceneComponent* AttachTarget = ResolveAttachTarget(ItemData->EquipSocketName);
	if (!AttachTarget || !ItemActor->AttachToComponent(
		AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ItemData->EquipSocketName))
	{
		ItemActor->Destroy();
		return false;
	}

	EquippedItemActor = ItemActor;
	if (!GrantSelectedItemAbilities())
	{
		ItemActor->Destroy();
		EquippedItemActor = nullptr;
		return false;
	}
	return true;
}

void UFTQuickSlotComponent::UnequipCurrentItem()
{
	RemoveSelectedItemAbilities();
	if (EquippedItemActor && GetOwner() && GetOwner()->HasAuthority())
	{
		EquippedItemActor->Destroy();
	}
	EquippedItemActor = nullptr;
}

bool UFTQuickSlotComponent::GrantSelectedItemAbilities()
{
	UFTItemDataAsset* ItemData = GetActiveItemData();
	if (!AbilitySystemComponent || !ItemData || !GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		return true;
	}
	return ItemData->WeaponDataAsset
		? GrantWeaponAbilities(ItemData->WeaponDataAsset)
		: GrantGenericItemAbility(ItemData);
}

bool UFTQuickSlotComponent::GrantGenericItemAbility(UFTItemDataAsset* ItemData)
{
	const TSubclassOf<UGameplayAbility> AbilityClass = ResolveAbilityClass(ItemData);
	if (!ItemData || !AbilityClass || !ItemData->PrimaryUseTag.IsValid())
	{
		return false;
	}

	FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, ItemData);
	Spec.GetDynamicSpecSourceTags().AddTag(TAG_FT_Input_Item_Primary);
	Spec.GetDynamicSpecSourceTags().AddTag(ItemData->PrimaryUseTag);
	const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
	if (Handle.IsValid())
	{
		GrantedAbilityHandles.Add(ItemData->PrimaryUseTag, Handle);
	}
	return Handle.IsValid();
}

bool UFTQuickSlotComponent::GrantWeaponAbilities(const UFTWeaponDataAsset* WeaponData)
{
	AFTWeaponActor* Weapon = Cast<AFTWeaponActor>(EquippedItemActor);
	if (!WeaponData || !Weapon)
	{
		return false;
	}

	for (const FFTWeaponActionDefinition& Definition : WeaponData->Actions)
	{
		TSubclassOf<UFTWeaponGameplayAbility> AbilityClass = Definition.AbilityClass;
		if (!AbilityClass)
		{
			AbilityClass = ResolveDefaultWeaponAbilityClass(GetActiveItemData());
		}
		if (!Definition.ActionTag.IsValid() || !AbilityClass ||
			GrantedAbilityHandles.Contains(Definition.ActionTag))
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, Weapon);
		Spec.GetDynamicSpecSourceTags().AddTag(TAG_FT_Input_Item_Primary);
		Spec.GetDynamicSpecSourceTags().AddTag(Definition.ActionTag);
		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
		if (Handle.IsValid())
		{
			GrantedAbilityHandles.Add(Definition.ActionTag, Handle);
		}
	}
	return !GrantedAbilityHandles.IsEmpty();
}

void UFTQuickSlotComponent::RemoveSelectedItemAbilities()
{
	if (AbilitySystemComponent && GetOwner() && GetOwner()->HasAuthority())
	{
		for (const TPair<FGameplayTag, FGameplayAbilitySpecHandle>& Pair : GrantedAbilityHandles)
		{
			AbilitySystemComponent->CancelAbilityHandle(Pair.Value);
			AbilitySystemComponent->ClearAbility(Pair.Value);
		}
	}
	GrantedAbilityHandles.Empty();
}

TSubclassOf<UGameplayAbility> UFTQuickSlotComponent::ResolveAbilityClass(
	const UFTItemDataAsset* ItemData) const
{
	if (!ItemData || !ItemData->PrimaryUseTag.IsValid() ||
		!ItemData->ItemTags.HasTagExact(ItemData->PrimaryUseTag))
	{
		return nullptr;
	}

	return ItemData->UseAbilityClass;
}

TSubclassOf<UFTWeaponGameplayAbility> UFTQuickSlotComponent::ResolveDefaultWeaponAbilityClass(
	const UFTItemDataAsset* ItemData) const
{
	if (!ItemData || !ItemData->ItemTags.HasTagExact(TAG_FT_Item_Type_Weapon))
	{
		return nullptr;
	}

	const bool bMelee = ItemData->ItemTags.HasTagExact(TAG_FT_Weapon_Type_Melee);
	const bool bHitScan = ItemData->ItemTags.HasTagExact(TAG_FT_Weapon_Type_HitScan);
	const bool bProjectile = ItemData->ItemTags.HasTagExact(TAG_FT_Weapon_Type_Projectile);
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

FGameplayAbilitySpecHandle UFTQuickSlotComponent::FindSelectedAbilityHandle(
	FGameplayTag ActionTag) const
{
	if (const FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(ActionTag))
	{
		return *Handle;
	}
	if (!AbilitySystemComponent)
	{
		return FGameplayAbilitySpecHandle();
	}

	const UFTItemDataAsset* ItemData = GetActiveItemData();
	const UObject* ExpectedSource = ItemData && ItemData->WeaponDataAsset
		? static_cast<const UObject*>(EquippedItemActor.Get())
		: static_cast<const UObject*>(ItemData);
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(ActionTag) &&
			(!ExpectedSource || Spec.SourceObject.Get() == ExpectedSource))
		{
			return Spec.Handle;
		}
	}
	return FGameplayAbilitySpecHandle();
}

UFTWeaponGameplayAbility* UFTQuickSlotComponent::GetActiveWeaponAbility(
	FGameplayTag ActionTag) const
{
	if (!AbilitySystemComponent)
	{
		return nullptr;
	}

	const FGameplayAbilitySpecHandle Handle = FindSelectedAbilityHandle(ActionTag);
	FGameplayAbilitySpec* Spec = Handle.IsValid()
		? AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
		: nullptr;
	return Spec ? Cast<UFTWeaponGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
}

int32 UFTQuickSlotComponent::ResolveSlotIndex(FGameplayTag InputTag) const
{
	if (InputTag.MatchesTagExact(TAG_FT_Input_QuickSlot_1)) return 0;
	if (InputTag.MatchesTagExact(TAG_FT_Input_QuickSlot_2)) return 1;
	if (InputTag.MatchesTagExact(TAG_FT_Input_QuickSlot_3)) return 2;
	if (InputTag.MatchesTagExact(TAG_FT_Input_QuickSlot_4)) return 3;
	return INDEX_NONE;
}

USceneComponent* UFTQuickSlotComponent::ResolveAttachTarget(FName SocketName) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(OwnerActor))
	{
		Mesh = Character->GetMesh();
	}
	if (!Mesh)
	{
		Mesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	}
	return Mesh && Mesh->DoesSocketExist(SocketName)
		? Cast<USceneComponent>(Mesh)
		: OwnerActor->GetRootComponent();
}
