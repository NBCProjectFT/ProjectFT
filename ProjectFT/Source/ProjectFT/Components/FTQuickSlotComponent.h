#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "FTQuickSlotComponent.generated.h"

class AFTItemActor;
class UAbilitySystemComponent;
class UFTItemDataAsset;
class UGameplayAbility;
class UFTWeaponGameplayAbility;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTQuickSlotEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|QuickSlot")
	TObjectPtr<UFTItemDataAsset> ItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|QuickSlot", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	bool IsEmpty() const { return !ItemData || Quantity <= 0; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuickSlotsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActiveQuickSlotChangedSignature);

/**
 * Owns four item shortcuts. It selects/equips an item and grants the GAS ability
 * mapped from the item's action definitions. Inventory ownership can replace the
 * temporary ItemData/Quantity entries later without changing input flow.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTQuickSlotComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool AssignItemToSlot(int32 SlotIndex, UFTItemDataAsset* ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool ClearSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool SelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool SelectSlotByInputTag(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool HandleInputTag(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool UseSelectedItem();

	void NotifyWeaponActionWindowBegin(FGameplayTag ActionTag);
	void NotifyWeaponActionWindowTick(FGameplayTag ActionTag);
	void NotifyWeaponActionWindowEnd(FGameplayTag ActionTag);

	/** Called by an item ability after a successful consumable use. */
	UFUNCTION(BlueprintCallable, Category = "FT|QuickSlot")
	bool ConsumeSelectedItem(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "FT|QuickSlot")
	TArray<FFTQuickSlotEntry> GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "FT|QuickSlot")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "FT|QuickSlot")
	UFTItemDataAsset* GetActiveItemData() const;

	UFUNCTION(BlueprintPure, Category = "FT|QuickSlot")
	AFTItemActor* GetEquippedItemActor() const { return EquippedItemActor; }

	UPROPERTY(BlueprintAssignable, Category = "FT|QuickSlot")
	FQuickSlotsChangedSignature OnQuickSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "FT|QuickSlot")
	FActiveQuickSlotChangedSignature OnActiveQuickSlotChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Slots,
		Category = "FT|QuickSlot", meta = (EditFixedSize))
	TArray<FFTQuickSlotEntry> Slots;

	/** One shared actor class used for every item and weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|QuickSlot")
	TSubclassOf<AFTItemActor> ItemActorClass;

	/** Temporary editor test item assigned to slot 0 when the slot is empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|QuickSlot|Test")
	TObjectPtr<UFTItemDataAsset> TestItemData;

private:
	UFUNCTION(Server, Reliable)
	void ServerAssignItemToSlot(int32 SlotIndex, UFTItemDataAsset* ItemData, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerClearSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSelectSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerConsumeSelectedItem(int32 Amount);

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

	bool EquipSelectedItem();
	void UnequipCurrentItem();
	bool GrantSelectedItemAbilities();
	bool GrantItemActions(UFTItemDataAsset* ItemData);
	void RemoveSelectedItemAbilities();
	TSubclassOf<UGameplayAbility> ResolveAbilityClassForAction(
		const UFTItemDataAsset* ItemData, FGameplayTag ActionTag) const;
	FGameplayAbilitySpecHandle FindSelectedAbilityHandle(FGameplayTag ActionTag) const;
	UFTWeaponGameplayAbility* GetActiveWeaponAbility(FGameplayTag ActionTag) const;
	int32 ResolveSlotIndex(FGameplayTag InputTag) const;
	USceneComponent* ResolveAttachTarget(FName SocketName) const;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex, Transient)
	int32 ActiveSlotIndex = INDEX_NONE;

	UPROPERTY(Replicated, Transient)
	TObjectPtr<AFTItemActor> EquippedItemActor;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	TMap<FGameplayTag, FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
