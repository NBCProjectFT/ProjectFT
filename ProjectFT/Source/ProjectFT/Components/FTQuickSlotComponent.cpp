#include "FTQuickSlotComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
// #include "ProjectFT/AbilitySystem/Abilities/FTGA_UseHealPotion.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_UseItem.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/AbilitySystem/Abilities/WeaponAbility/FTHitScanWeaponGameplayAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/WeaponAbility/FTMeleeWeaponGameplayAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/WeaponAbility/FTProjectileWeaponGameplayAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/WeaponAbility/FTWeaponGameplayAbility.h"

namespace
{
FGameplayTag ResolveInputTag(const FFTItemActionDefinition& Action)
{
	return Action.InputTag.IsValid() ? Action.InputTag : TAG_FT_Input_Item_Primary;
}

FGameplayTag ResolveActionTag(const FFTItemActionDefinition& Action)
{
	if (Action.ActionTag.IsValid())
	{
		return Action.ActionTag;
	}
	return TAG_FT_Weapon_Action_Primary;
}
}

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
	if (GetOwner() && GetOwner()->HasAuthority() && TestItemData && Slots[0].IsEmpty())
	{
		AssignItemToSlot(0, TestItemData, 1);
	}
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

	UE_LOG(LogTemp, Warning, TEXT("SelectSlot: %d, IsEmpty: %d"), SlotIndex, Slots[SlotIndex].IsEmpty());
	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipCurrentItem();
		ActiveSlotIndex = INDEX_NONE;
		OnActiveQuickSlotChanged.Broadcast();
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

	const FFTItemActionDefinition* Action = ItemData->Actions.FindByPredicate(
		[](const FFTItemActionDefinition& Candidate)
		{
			return ResolveInputTag(Candidate).MatchesTagExact(TAG_FT_Input_Item_Primary);
		});
	if (!Action)
	{
		return false;
	}
	const FGameplayTag ActionTag = ResolveActionTag(*Action);
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

	TSubclassOf<AFTItemActor> ActorClass = ItemActorClass;
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

	ItemActor->InitializeFromItemData(ItemData);
	UGameplayStatics::FinishSpawningActor(ItemActor, GetOwner()->GetActorTransform());
	ItemActor->SetEquipped(true);

	USceneComponent* AttachTarget = ResolveAttachTarget(ItemData->EquipSocketName);
	if (!AttachTarget || !ItemActor->AttachToComponent(
		AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ItemData->EquipSocketName))
	{
		ItemActor->Destroy();
		return false;
	}
	ItemActor->SetActorRelativeTransform(ItemData->EquipRelativeTransform);

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
	return GrantItemActions(ItemData);
}

bool UFTQuickSlotComponent::GrantItemActions(UFTItemDataAsset* ItemData)
{
	AFTItemActor* ItemActor = EquippedItemActor;
	if (!ItemData || !ItemActor)
	{
		return false;
	}

	for (const FFTItemActionDefinition& Action : ItemData->Actions)
	{
		const FGameplayTag InputTag = ResolveInputTag(Action);
		const FGameplayTag ActionTag = ResolveActionTag(Action);
		TSubclassOf<UGameplayAbility> AbilityClass =
			ResolveAbilityClassForAction(ItemData, ActionTag);
		if (!AbilityClass || GrantedAbilityHandles.Contains(ActionTag))
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, ItemActor);
		Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
		Spec.GetDynamicSpecSourceTags().AddTag(ActionTag);
		Spec.GetDynamicSpecSourceTags().AppendTags(ItemData->ItemTags);
		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
		if (Handle.IsValid())
		{
			GrantedAbilityHandles.Add(ActionTag, Handle);
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

TSubclassOf<UGameplayAbility> UFTQuickSlotComponent::ResolveAbilityClassForAction(
	const UFTItemDataAsset* ItemData, FGameplayTag ActionTag) const
{
	if (!ItemData || !ActionTag.IsValid())
	{
		return nullptr;
	}

	if (ActionTag.MatchesTagExact(TAG_FT_Item_Action_Heal))
	{
		// return UFTGA_UseHealPotion::StaticClass();
	}

	if (ActionTag.MatchesTagExact(TAG_FT_Item_Action_Throw))
	{
		return UFTProjectileWeaponGameplayAbility::StaticClass();
	}

	if (ActionTag.MatchesTagExact(TAG_FT_Weapon_Action_Primary) ||
		ActionTag.MatchesTagExact(TAG_FT_Weapon_Action_Secondary))
	{
		if (!ItemData->ItemTags.HasTagExact(TAG_FT_Item_Type_Weapon))
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

	return nullptr;
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

	const UObject* ExpectedSource = EquippedItemActor.Get();
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
