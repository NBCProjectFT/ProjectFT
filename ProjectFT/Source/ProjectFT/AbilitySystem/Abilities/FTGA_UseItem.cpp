#include "FTGA_UseItem.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTQuickSlotComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

UFTGA_UseItem::UFTGA_UseItem()
{
	CooldownGameplayEffectClass = UFTGE_Cooldown::StaticClass();
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);
}

void UFTGA_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!PrepareItemUse() || !CommitAbility(Handle, ActorInfo, ActivationInfo) ||
		!PlayItemMontage())
	{
		FinishItemUse(true);
		return;
	}

	const float CastTime = GetUseCastTime();
	if (CastTime > 0.0f)
	{
		UAbilityTask_WaitDelay* CastTask = UAbilityTask_WaitDelay::WaitDelay(this, CastTime);
		CastTask->OnFinish.AddDynamic(this, &ThisClass::OnCastFinished);
		CastTask->ReadyForActivation();
		return;
	}

	PerformItemUse();
}

void UFTGA_UseItem::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	PlayedMontageDuration = 0.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo,
		bReplicateEndAbility, bWasCancelled);
}

void UFTGA_UseItem::OnCastFinished()
{
	PerformItemUse();
}

void UFTGA_UseItem::PerformItemUse()
{
	if (!ExecuteItemUse())
	{
		FinishItemUse(true);
		return;
	}

	OnItemConsumed();
	if (ShouldEndImmediately())
	{
		FinishItemUse();
	}
}

bool UFTGA_UseItem::ExecuteItemUse()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<UGameplayEffect> EffectClass = GetUseEffectClass();
	if (!ASC || !EffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetCurrentSourceObject());
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		EffectClass, GetAbilityLevel(), Context);
	if (!Spec.IsValid())
	{
		return false;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return true;
}

void UFTGA_UseItem::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float Cooldown = GetUseCooldown();
	if (Cooldown <= 0.0f)
	{
		return;
	}

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(),
		GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Cooldown, Cooldown);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

void UFTGA_UseItem::OnItemConsumed()
{
	const UFTItemDataAsset* ItemData = GetItemData();
	if (ItemData && !ItemData->bConsumeOnUse)
	{
		return;
	}

	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UFTQuickSlotComponent* QuickSlot =
			Avatar->FindComponentByClass<UFTQuickSlotComponent>())
		{
			QuickSlot->ConsumeSelectedItem(1);
		}
	}
}

const UFTItemDataAsset* UFTGA_UseItem::GetItemData() const
{
	UObject* SourceObject = GetCurrentSourceObject();
	if (const UFTItemDataAsset* DataAsset = Cast<UFTItemDataAsset>(SourceObject))
	{
		return DataAsset;
	}
	if (const AFTItemActor* ItemActor = Cast<AFTItemActor>(SourceObject))
	{
		return ItemActor->ItemData;
	}
	return nullptr;
}

float UFTGA_UseItem::GetUseCastTime() const
{
	const UFTItemDataAsset* ItemData = GetItemData();
	return ItemData ? ItemData->UseCastTime : CastTimeSeconds;
}

float UFTGA_UseItem::GetUseCooldown() const
{
	const UFTItemDataAsset* ItemData = GetItemData();
	return ItemData ? ItemData->UseCooldown : CooldownSeconds;
}

TSubclassOf<UGameplayEffect> UFTGA_UseItem::GetUseEffectClass() const
{
	const UFTItemDataAsset* ItemData = GetItemData();
	return ItemData && ItemData->UseEffectClass
		? ItemData->UseEffectClass
		: ItemEffect;
}

bool UFTGA_UseItem::PlayItemMontage()
{
	PlayedMontageDuration = 0.0f;
	const UFTItemDataAsset* ItemData = GetItemData();
	if (!ItemData || ItemData->UseMontage.IsNull())
	{
		return true;
	}

	UAnimMontage* Montage = ItemData->UseMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = GetCurrentActorInfo()
		? GetCurrentActorInfo()->GetAnimInstance()
		: nullptr;
	if (!Montage || !AnimInstance)
	{
		return false;
	}

	PlayedMontageDuration = AnimInstance->Montage_Play(Montage);
	return PlayedMontageDuration > 0.0f;
}

void UFTGA_UseItem::FinishItemUse(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
		GetCurrentActivationInfo(), true, bWasCancelled);
}
