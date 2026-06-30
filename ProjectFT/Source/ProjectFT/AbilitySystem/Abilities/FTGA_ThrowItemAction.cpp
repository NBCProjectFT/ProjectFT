#include "FTGA_ThrowItemAction.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTThrowDataAsset.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "ProjectFT/Item/FTProjectileActor.h"
#include "ProjectFT/Struct/FTThrowActorStruct.h"
#include "ProjectFT/Struct/FTProjectileActorStruct.h"

UFTGA_ThrowItemAction::UFTGA_ThrowItemAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 조준(활성) 중 소유자에게 상태 태그를 부여(ActivationOwnedTags) → "아이템 동작 진행 중?" 가드 질의에 쓰인다.
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);

	// 식별 AssetTag(조준형). CancelAbilities는 AssetTags를 매칭한다. .Aimed는 이동 취소(.Channeled) 대상이 아니라
	// "달리며 던지기"가 가능하고, 퀵슬롯 전환 시엔 부모 Ability.ItemUse 질의에 걸려 취소된다.
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(TAG_FT_Ability_ItemUse_Aimed);
		SetAssetTags(AssetTags);
	}
	
	FAbilityTriggerData UseTrigger;
	UseTrigger.TriggerTag = TAG_FT_Event_UseItem;
	UseTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(UseTrigger);

}

void UFTGA_ThrowItemAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveThrowData = Cast<UFTThrowDataAsset>(ActiveItemData);

	if (!ActiveItemData || !ActiveThrowData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: invalid throw item data."));
		EndThrowAbility(true);
		return;
	}

	ProjectileActorData = ActiveThrowData->ThrowActorData.ProjectileItemData;

	if (!ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: ProjectileItemData is null."));
		EndThrowAbility(true);
		return;
	}

	const FFTThrowActorStruct* ThrowData = GetThrowActorData();
	const FFTProjectileActorStruct* ProjectileData = GetProjectileActorData();

	if (!ThrowData || !ProjectileData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: throw or projectile data is null."));
		EndThrowAbility(true);
		return;
	}

	if (!ProjectileData->ProjectileActorClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: ProjectileActorClass is null."));
		EndThrowAbility(true);
		return;
	}

	if (!EnsureProjectileAbilityGranted())
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: EnsureProjectileAbilityGranted failed."));
		EndThrowAbility(true);
		return;
	}

	if (!bIsHoldingProjectile)
	{
		if (!StartHoldingProjectile())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: StartHoldingProjectile failed."));
			EndThrowAbility(true);
			return;
		}

		if (ThrowData->PrepareMontage)
		{
			if (UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Play(ThrowData->PrepareMontage, 1.0f);
			}
		}

		if (!WaitForUseReleased())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: WaitForUseReleased failed."));
			EndThrowAbility(true);
		}

		return;
	}

	HandleUseReleasedEvent(FGameplayEventData());
}

void UFTGA_ThrowItemAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (HeldProjectile)
	{
		ClearHeldProjectile();
	}

	ActiveItemData = nullptr;
	ActiveThrowData = nullptr;
	ProjectileActorData = nullptr;
	ActiveThrowMontage = nullptr;
	bIsHoldingProjectile = false;
	bWaitingForThrowRelease = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTGA_ThrowItemAction::HandleUseReleasedEvent(FGameplayEventData Payload)
{
	const FFTThrowActorStruct* ThrowData = GetThrowActorData();
	if (!ThrowData)
	{
		EndThrowAbility(true);
		return;
	}

	if (ThrowData->ThrowMontage)
	{
		if (!WaitForThrowRelease())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: WaitForThrowRelease failed."));
			EndThrowAbility(true);
			return;
		}

		ActiveThrowMontage = ThrowData->ThrowMontage;
		bWaitingForThrowRelease = true;

		UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
		if (!AnimInstance)
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: AnimInstance is null."));
			EndThrowAbility(true);
			return;
		}

		const float MontageLength = AnimInstance->Montage_Play(ThrowData->ThrowMontage, 1.0f);
		if (MontageLength <= 0.0f)
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: ThrowMontage did not play. Montage=%s"),
				*GetNameSafe(ThrowData->ThrowMontage));
			EndThrowAbility(true);
			return;
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UFTGA_ThrowItemAction::HandleThrowMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, ThrowData->ThrowMontage);

		return;
	}

	// ThrowMontage가 없으면 디버그용으로 마우스를 뗀 순간 바로 던진다.
	if (!ReleaseHeldProjectile())
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: released ReleaseHeldProjectile failed."));
		EndThrowAbility(true);
		return;
	}

	EndThrowAbility(false);
}

void UFTGA_ThrowItemAction::HandleThrowReleaseEvent(FGameplayEventData Payload)
{
	bWaitingForThrowRelease = false;
	ActiveThrowMontage = nullptr;

	if (!ReleaseHeldProjectile())
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: notify ReleaseHeldProjectile failed."));
		EndThrowAbility(true);
		return;
	}

	EndThrowAbility(false);
}

void UFTGA_ThrowItemAction::HandleThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bWaitingForThrowRelease || Montage != ActiveThrowMontage)
	{
		return;
	}

	UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction cancelled: ThrowRelease notify was not received. Montage=%s Interrupted=%d"),
		*GetNameSafe(Montage),
		bInterrupted ? 1 : 0);

	EndThrowAbility(true);
}

bool UFTGA_ThrowItemAction::StartHoldingProjectile()
{
	const FFTThrowActorStruct* ThrowData = GetThrowActorData();
	const FFTProjectileActorStruct* ProjectileData = GetProjectileActorData();

	if (!ThrowData || !ProjectileData || !ProjectileActorData)
	{
		return false;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UMeshComponent* AttachMesh = ResolveAttachMesh(Avatar, ThrowData->AttachSocketName);
	if (!AttachMesh)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: attach socket not found. Socket=%s"),
			*ThrowData->AttachSocketName.ToString());
		return false;
	}

	const FTransform SpawnTransform = AttachMesh->GetSocketTransform(
		ThrowData->AttachSocketName,
		ERelativeTransformSpace::RTS_World
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	HeldProjectile = World->SpawnActor<AFTProjectileActor>(
		ProjectileData->ProjectileActorClass,
		SpawnTransform,
		SpawnParams
	);

	if (!HeldProjectile)
	{
		return false;
	}

	HeldProjectile->InitializeHeldProjectile(
		ProjectileActorData,
		Avatar
	);

	HeldProjectile->AttachToComponent(
		AttachMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ThrowData->AttachSocketName
	);

	bIsHoldingProjectile = true;

	return true;
}

bool UFTGA_ThrowItemAction::WaitForUseReleased()
{
	UAbilityTask_WaitGameplayEvent* ReleaseTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_FT_Event_UseReleased,
			nullptr,
			true,
			true
		);

	if (!ReleaseTask)
	{
		return false;
	}

	ReleaseTask->EventReceived.AddDynamic(this, &UFTGA_ThrowItemAction::HandleUseReleasedEvent);
	ReleaseTask->ReadyForActivation();
	return true;
}

bool UFTGA_ThrowItemAction::WaitForThrowRelease()
{
	UAbilityTask_WaitGameplayEvent* ThrowReleaseTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_FT_Event_ThrowRelease,
			nullptr,
			true,
			true
		);

	if (!ThrowReleaseTask)
	{
		return false;
	}

	ThrowReleaseTask->EventReceived.AddDynamic(this, &UFTGA_ThrowItemAction::HandleThrowReleaseEvent);
	ThrowReleaseTask->ReadyForActivation();
	return true;
}

bool UFTGA_ThrowItemAction::ReleaseHeldProjectile()
{
	if (!HeldProjectile || !ProjectileActorData)
	{
		return false;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: CommitAbility failed."));
		return false;
	}

	const FVector ThrowDirection = GetViewDirection();

	HeldProjectile->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform
	);

	HeldProjectile->ReleaseProjectile(ThrowDirection);

	OnItemConsumed();

	HeldProjectile = nullptr;
	bIsHoldingProjectile = false;
	bWaitingForThrowRelease = false;
	ActiveThrowMontage = nullptr;

	return true;
}

bool UFTGA_ThrowItemAction::EnsureProjectileAbilityGranted()
{
	if (!ProjectileActorData)
	{
		return false;
	}

	TSubclassOf<UGameplayAbility> ProjectileAbilityClass =
		ProjectileActorData->ItemData.UseData.UseAbility;

	if (!ProjectileAbilityClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: Projectile UseAbility is null."));
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		if (AbilitySpec.Ability->GetClass() == ProjectileAbilityClass)
		{
			return true;
		}
	}

	if (CurrentActorInfo && !CurrentActorInfo->IsNetAuthority())
	{
		return true;
	}

	FGameplayAbilitySpec AbilitySpec(
		ProjectileAbilityClass,
		1,
		INDEX_NONE,
		ProjectileActorData
	);

	ASC->GiveAbility(AbilitySpec);

	return true;
}

void UFTGA_ThrowItemAction::EndThrowAbility(bool bWasCancelled)
{
	if (bWasCancelled && HeldProjectile)
	{
		ClearHeldProjectile();
	}

	if (!bIsHoldingProjectile)
	{
		ActiveItemData = nullptr;
		ActiveThrowData = nullptr;
		ProjectileActorData = nullptr;
	}

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled
	);
}

void UFTGA_ThrowItemAction::ClearHeldProjectile()
{
	if (HeldProjectile)
	{
		HeldProjectile->Destroy();
		HeldProjectile = nullptr;
	}

	bIsHoldingProjectile = false;
}

const FFTThrowActorStruct* UFTGA_ThrowItemAction::GetThrowActorData() const
{
	return ActiveThrowData ? &ActiveThrowData->ThrowActorData : nullptr;
}

const FFTProjectileActorStruct* UFTGA_ThrowItemAction::GetProjectileActorData() const
{
	return ProjectileActorData ? &ProjectileActorData->ProjectileActorData : nullptr;
}

UMeshComponent* UFTGA_ThrowItemAction::ResolveAttachMesh(AActor* Avatar, FName RequiredSocketName) const
{
	if (!Avatar || RequiredSocketName.IsNone())
	{
		return nullptr;
	}

	TArray<UMeshComponent*> AvatarMeshes;
	Avatar->GetComponents<UMeshComponent>(AvatarMeshes);

	for (UMeshComponent* MeshComponent : AvatarMeshes)
	{
		if (MeshComponent && MeshComponent->DoesSocketExist(RequiredSocketName))
		{
			return MeshComponent;
		}
	}

	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor)
		{
			continue;
		}

		TArray<UMeshComponent*> AttachedMeshes;
		AttachedActor->GetComponents<UMeshComponent>(AttachedMeshes);

		for (UMeshComponent* MeshComponent : AttachedMeshes)
		{
			if (MeshComponent && MeshComponent->DoesSocketExist(RequiredSocketName))
			{
				return MeshComponent;
			}
		}
	}

	return nullptr;
}

FVector UFTGA_ThrowItemAction::GetViewDirection() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FVector::ForwardVector;
	}

	FVector ViewLocation = Avatar->GetActorLocation();
	FRotator ViewRotation = Avatar->GetActorRotation();

	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
		else
		{
			Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		}
	}
	else
	{
		Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	return ViewRotation.Vector().GetSafeNormal();
}
