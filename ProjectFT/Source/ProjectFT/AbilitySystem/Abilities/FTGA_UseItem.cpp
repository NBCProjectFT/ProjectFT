// Fill out your copyright notice in the Description page of Project Settings.

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

UFTGA_UseItem::UFTGA_UseItem()
{
	// 쿨다운은 공용 쿨다운 GE로 처리(지속시간은 ApplyCooldown에서 주입). 아이템별 분리가 필요하면 파생에서 교체.
	CooldownGameplayEffectClass = UFTGE_Cooldown::StaticClass();

	// 시전(활성) 중 소유자에게 상태 태그를 부여 → 시전 중 이동하면 캐릭터가 이 태그로 어빌리티를 취소한다.
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);
}

void UFTGA_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!PlayItemMontage())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 쿨다운 차단은 CanActivateAbility(CheckCooldown)에서 이미 걸러진다. 여기서는 시전 흐름만 진행한다.
	const float EffectiveCastTime = GetUseCastTime();
	if (EffectiveCastTime > 0.0f)
	{
		// 시전시간 동안 대기 후 효과 적용. 시전 중에는 동일 어빌리티가 활성 상태라 재사용이 막힌다.
		UAbilityTask_WaitDelay* CastTask = UAbilityTask_WaitDelay::WaitDelay(this, EffectiveCastTime);
		CastTask->OnFinish.AddDynamic(this, &UFTGA_UseItem::OnCastFinished);
		CastTask->ReadyForActivation();
	}
	else
	{
		FinishUse();
	}
}

void UFTGA_UseItem::OnCastFinished()
{
	FinishUse();
}

void UFTGA_UseItem::FinishUse()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<UGameplayEffect> EffectClass = GetUseEffectClass();
	if (!ASC || !EffectClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 효과 적용(가이드 4.4 패턴: 컨텍스트에 근원 등록 → 스펙 생성 → 자신에게 적용). 만들어 둔 GE를 그대로 재사용.
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);
	if (EffectSpec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}

	// 쿨다운 적용(재정의된 ApplyCooldown이 CooldownSeconds를 주입; 0이면 아무것도 하지 않음).
	ApplyCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

	OnItemConsumed();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UFTGA_UseItem::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float EffectiveCooldown = GetUseCooldown();
	if (EffectiveCooldown <= 0.0f)
	{
		return; // 쿨다운 없음.
	}

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	// 공용 쿨다운 GE에 이 어빌리티의 쿨다운 시간을 SetByCaller로 주입해 적용한다.
	const FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (CooldownSpec.IsValid())
	{
		CooldownSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Cooldown, EffectiveCooldown);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
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
	return Cast<UFTItemDataAsset>(GetCurrentSourceObject());
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

bool UFTGA_UseItem::PlayItemMontage() const
{
	const UFTItemDataAsset* ItemData = GetItemData();
	if (!ItemData || ItemData->UseMontage.IsNull())
	{
		return true;
	}

	UAnimMontage* Montage = ItemData->UseMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = GetCurrentActorInfo()
		? GetCurrentActorInfo()->GetAnimInstance()
		: nullptr;
	return Montage && AnimInstance && AnimInstance->Montage_Play(Montage) > 0.0f;
}
