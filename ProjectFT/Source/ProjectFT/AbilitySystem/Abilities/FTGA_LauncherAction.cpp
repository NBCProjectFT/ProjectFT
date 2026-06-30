// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_LauncherAction.h"

#include "Animation/AnimInstance.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTLauncherDataAsset.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "ProjectFT/Item/FTProjectileActor.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTLauncherActionStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTProjectileActorStruct.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

UFTGA_LauncherAction::UFTGA_LauncherAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_LauncherAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveLauncherData = Cast<UFTLauncherDataAsset>(ActiveItemData);

	if (!ActiveItemData || !ActiveLauncherData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: invalid launcher item data."));
		EndLauncherAbility(true);
		return;
	}

	ProjectileActorData = Cast<UFTProjectileActorDataAsset>(
		ActiveLauncherData->LauncherActionData.ProjectileItemData
	);

	if (!ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: ProjectileItemData is not UFTProjectileActorDataAsset."));
		EndLauncherAbility(true);
		return;
	}

	if (!EnsureProjectileAbilityGranted())
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: EnsureProjectileAbilityGranted failed."));
		EndLauncherAbility(true);
		return;
	}
	
	const FFTLauncherActionStruct* LauncherData = GetLauncherActionData();
	const FFTProjectileActorStruct* ProjectileData = GetProjectileActorData();

	if (!LauncherData || !ProjectileData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: launcher or projectile data is null."));
		EndLauncherAbility(true);
		return;
	}

	if (!ProjectileData->ProjectileActorClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: ProjectileActorClass is null."));
		EndLauncherAbility(true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: Avatar is null."));
		EndLauncherAbility(true);
		return;
	}

	// 탄환으로 사용할 ProjectileActorDataAsset의 ItemId
	const FName ProjectileItemId = ProjectileActorData->ItemData.ItemId;
	
	if (ProjectileItemId.IsNone())
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: ProjectileItemId is none."));
		EndLauncherAbility(true);
		return;
	}

	// 인벤토리에 탄환 수량이 있는지 확인
	UFTInventoryComponent* InventoryComponent =
		Avatar->FindComponentByClass<UFTInventoryComponent>();

	if (!InventoryComponent)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: InventoryComponent is null."));
		EndLauncherAbility(true);
		return;
	}
	
	if (InventoryComponent->GetItemQuantity(ProjectileItemId) <= 0)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: no projectile item. ItemId=%s"),
			*ProjectileItemId.ToString());

		EndLauncherAbility(true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: CommitAbility failed."));
		EndLauncherAbility(true);
		return;
	}

	// 발사 성공여부
	const bool bFired = FireProjectile();
	
	if (!bFired)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: FireProjectile failed."));
		EndLauncherAbility(true);
		return;
	}

	// 발사 성공 후 탄환 아이템 1개 차감
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = Avatar;
	Payload.ItemId = ProjectileItemId;

	UGameplayMessageSubsystem::Get(Avatar).BroadcastMessage(
		TAG_FT_Event_ItemConsumed,
		Payload
	);

	if (LauncherData->AttackMontage)
	{
		if (UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(LauncherData->AttackMontage, 1.0f);
		}
	}

	EndLauncherAbility(false);
}

bool UFTGA_LauncherAction::FireProjectile()
{
	const FFTLauncherActionStruct* LauncherData = GetLauncherActionData();
	const FFTProjectileActorStruct* ProjectileData = GetProjectileActorData();

	if (!LauncherData || !ProjectileData || !ProjectileActorData)
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

	// 1. 카메라 / 컨트롤러 기준 발사 방향 구하기
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

	FVector SpawnLocation = ViewLocation;
	FRotator SpawnRotation = ViewRotation;

	// 2. 장착된 발사기 Mesh에서 Muzzle 소켓 찾기
	if (UMeshComponent* LauncherMesh = ResolveLauncherMesh(Avatar, LauncherData->MuzzleSocketName))
	{
		SpawnLocation = LauncherMesh->GetSocketLocation(LauncherData->MuzzleSocketName);
		SpawnRotation = ViewRotation;
	}

	const FVector FireDirection = SpawnRotation.Vector().GetSafeNormal();

	// 3. ProjectileActor 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFTProjectileActor* Projectile = World->SpawnActor<AFTProjectileActor>(
		ProjectileData->ProjectileActorClass,
		SpawnLocation,
		FireDirection.Rotation(),
		SpawnParams
	);
	
	if (!Projectile)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: projectile spawn failed."));
		return false;
	}

	// 4. ProjectileActor에 ProjectileActorDataAsset 자체를 전달
	Projectile->InitializeProjectile(
		ProjectileActorData,
		FireDirection,
		Avatar
	);

	return true;
}

bool UFTGA_LauncherAction::EnsureProjectileAbilityGranted()
{
	if (!ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("EnsureProjectileAbilityGranted failed: ProjectileActorData is null."));
		return false;
	}

	TSubclassOf<UGameplayAbility> ProjectileAbilityClass =
		ProjectileActorData->ItemData.UseData.UseAbility;

	if (!ProjectileAbilityClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("EnsureProjectileAbilityGranted failed: Projectile UseAbility is null."));
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		UE_LOG(LogFTItem, Warning, TEXT("EnsureProjectileAbilityGranted failed: ASC is null."));
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

	UE_LOG(LogFTItem, Log, TEXT("Projectile ability granted: %s"),
		*GetNameSafe(ProjectileAbilityClass.Get()));

	return true;
}

// 어빌리티 종료
void UFTGA_LauncherAction::EndLauncherAbility(bool bWasCancelled)
{
	ActiveItemData = nullptr;
	ActiveLauncherData = nullptr;
	ProjectileActorData = nullptr;

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled
	);
}

// FFTLauncherActionStruct
const FFTLauncherActionStruct* UFTGA_LauncherAction::GetLauncherActionData() const
{
	return ActiveLauncherData ? &ActiveLauncherData->LauncherActionData : nullptr;
}

const FFTProjectileActorStruct* UFTGA_LauncherAction::GetProjectileActorData() const
{
	return ProjectileActorData ? &ProjectileActorData->ProjectileActorData : nullptr;
}

UMeshComponent* UFTGA_LauncherAction::ResolveLauncherMesh(AActor* Avatar, FName RequiredSocketName) const
{
	if (!Avatar || RequiredSocketName.IsNone())
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor)
		{
			continue;
		}

		TArray<UMeshComponent*> MeshComponents;
		AttachedActor->GetComponents<UMeshComponent>(MeshComponents);

		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (!MeshComponent)
			{
				continue;
			}

			if (MeshComponent->DoesSocketExist(RequiredSocketName))
			{
				return MeshComponent;
			}
		}
	}

	return nullptr;
}