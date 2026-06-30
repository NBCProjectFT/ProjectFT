#include "FTGA_ThrowItemAction.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
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

	FAbilityTriggerData UseTrigger;
	UseTrigger.TriggerTag = TAG_FT_Event_UseItem;
	UseTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(UseTrigger);

	FAbilityTriggerData ReleaseTrigger;
	ReleaseTrigger.TriggerTag = TAG_FT_Event_ThrowRelease;
	ReleaseTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(ReleaseTrigger);
}

void UFTGA_ThrowItemAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData &&
		TriggerEventData->EventTag.MatchesTagExact(TAG_FT_Event_ThrowRelease))
	{
		if (!ReleaseHeldProjectile())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: ReleaseHeldProjectile failed."));
			EndThrowAbility(true);
			return;
		}

		EndThrowAbility(false);
		return;
	}

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

		// 들고 있는 상태는 유지하고, 어빌리티 실행만 종료한다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (ThrowData->ThrowMontage)
	{
		if (UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(ThrowData->ThrowMontage, 1.0f);
		}

		// 실제 발사는 AnimNotify(TAG_FT_Event_ThrowRelease)에서 처리한다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ThrowMontage가 없으면 디버그용으로 즉시 던진다.
	if (!ReleaseHeldProjectile())
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: immediate ReleaseHeldProjectile failed."));
		EndThrowAbility(true);
		return;
	}

	EndThrowAbility(false);
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
		HeldProjectile->Destroy();
		HeldProjectile = nullptr;
		bIsHoldingProjectile = false;
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
