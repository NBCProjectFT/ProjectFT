#include "FTGA_UseItem.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h" //삭제 예정
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTQuickSlotComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

UFTGA_UseItem::UFTGA_UseItem()
{
	CooldownGameplayEffectClass = UFTGE_Cooldown::StaticClass(); //삭제 예정
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);

	// 아이템 사용 입력이 보내는 GameplayEvent(Event.UseItem)로 발동된다(페이로드 = 대상 UFTItemDataAsset).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 발동 페이로드의 아이템 데이터에서 사용 정보를 읽어 캐싱한다(베이스). 없으면 사용 불가로 취소.
	if (!CacheActiveItem(TriggerEventData))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 쿨다운 차단은 CanActivateAbility(CheckCooldown)에서 이미 걸러진다. 여기서는 시전 흐름만 진행한다.
	if (ActiveUseData.CastTimeSeconds > 0.0f)
	{
		// 시전시간 동안 대기 후 효과 적용. 시전 중에는 동일 어빌리티가 활성 상태라 재사용이 막힌다.
		UAbilityTask_WaitDelay* CastTask = UAbilityTask_WaitDelay::WaitDelay(this, ActiveUseData.CastTimeSeconds);
		CastTask->OnFinish.AddDynamic(this, &UFTGA_UseItem::OnCastFinished);

	
/*//플레이어가 몽타주 직접 사용 중	
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
		// 여기까지

		*/
		CastTask->ReadyForActivation();
		return;
	}
	

	// PerformItemUse();
}

void UFTGA_UseItem::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// PlayedMontageDuration = 0.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo,
		bReplicateEndAbility, bWasCancelled);
}

void UFTGA_UseItem::OnCastFinished()
{
	// PerformItemUse();
}

void UFTGA_UseItem::PerformItemUse()
{
	// 비용/쿨다운을 표준 경로로 커밋(효과 적용 "전"에 → 비용 부족이면 효과 없이 취소).
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		// FinishItemUse(true);
		return;
	}

	// UseEffects를 자신에게 적용(베이스 헬퍼, TargetData 없음 → Owner).
	ApplyUseEffects(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

	OnItemConsumed();
	if (ShouldEndImmediately())
	{
		// FinishItemUse();
	}
}
/*
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
	const FFTItemActionDefinition* Action = GetItemActionDefinition();
	if (!Action || !Action->bConsumeOnUse)
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

const FFTItemActionDefinition* UFTGA_UseItem::GetItemActionDefinition() const
{
	const UFTItemDataAsset* ItemData = GetItemData();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC
		? ASC->FindAbilitySpecFromHandle(GetCurrentAbilitySpecHandle())
		: nullptr;
	if (!ItemData || !Spec)
	{
		return nullptr;
	}

	return ItemData->Actions.FindByPredicate(
		[Spec](const FFTItemActionDefinition& Action)
		{
			const FGameplayTag ActionTag = Action.ActionTag.IsValid()
				? Action.ActionTag
				: TAG_FT_Weapon_Action_Primary;
			return Spec->GetDynamicSpecSourceTags().HasTagExact(ActionTag);
		});
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
	return CastTimeSeconds;
}

float UFTGA_UseItem::GetUseCooldown() const
{
	return CooldownSeconds;
}

TSubclassOf<UGameplayEffect> UFTGA_UseItem::GetUseEffectClass() const
{
	const FFTItemActionDefinition* Action = GetItemActionDefinition();
	return Action && Action->EffectClass
		? Action->EffectClass
		: ItemEffect;
}

bool UFTGA_UseItem::PlayItemMontage()
{
	PlayedMontageDuration = 0.0f;
	const FFTItemActionDefinition* Action = GetItemActionDefinition();
	if (!Action || Action->Montage.IsNull())
	{
		return true;
	}

	UAnimMontage* Montage = Action->Montage.LoadSynchronous();
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
*/