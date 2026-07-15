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
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UFTGA_LauncherAction::UFTGA_LauncherAction()
{
	// 발사기 데이터와 Projectile 데이터 캐시를 인스턴스에 보관한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 발사기는 아이템 사용 입력(Event.UseItem)으로 즉시 발사된다.
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

	// 사용한 아이템을 발사기 DataAsset으로 캐싱한다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveLauncherData = Cast<UFTLauncherDataAsset>(ActiveItemData);

	// 이 Ability는 UFTLauncherDataAsset 전용이다.
	if (!ActiveItemData || !ActiveLauncherData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: invalid launcher item data."));
		EndLauncherAbility(true);
		return;
	}

	ProjectileActorData = Cast<UFTProjectileActorDataAsset>(
		ActiveLauncherData->LauncherActionData.ProjectileItemData
	);

	// Launcher DataAsset은 실제 스폰될 ProjectileActorDataAsset을 참조해야 한다.
	if (!ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: ProjectileItemData is not UFTProjectileActorDataAsset."));
		EndLauncherAbility(true);
		return;
	}

	// ProjectileActor가 충돌 후 TargetHit 이벤트를 보냈을 때 받을 Ability를 미리 부여한다.
	if (!EnsureProjectileAbilityGranted())
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: EnsureProjectileAbilityGranted failed."));
		EndLauncherAbility(true);
		return;
	}
	
	// 발사기 설정과 투사체 설정을 모두 확인한 뒤에 실제 발사 단계로 넘어간다.
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

	// 탄환으로 사용할 ProjectileActorDataAsset의 ItemId.
	// Launcher 자체 아이템이 아니라 ProjectileActorDataAsset 아이템 수량을 탄약처럼 검사한다.
	const FName ProjectileItemId = ProjectileActorData->ItemData.ItemId;
	
	if (ProjectileItemId.IsNone())
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: ProjectileItemId is none."));
		EndLauncherAbility(true);
		return;
	}

	// 인벤토리에 탄환 수량이 있는지 확인한다.
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

	bHasFiredInThisActivation = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: CommitAbility failed."));
		EndLauncherAbility(true);
		return;
	}

	if (LauncherData->AttackMontage)
	{
		// AnimNotify 대기 태스크 생성 (태그: Event.ThrowRelease)
		UAbilityTask_WaitGameplayEvent* WaitReleaseTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				TAG_FT_Event_ThrowRelease,
				nullptr,
				true,
				true
			);
		
		if (WaitReleaseTask)
		{
			WaitReleaseTask->EventReceived.AddDynamic(this, &UFTGA_LauncherAction::HandleThrowReleaseEvent);
			WaitReleaseTask->ReadyForActivation();
		}

		// 몽타주 재생 및 대기 태스크 생성
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				TEXT("LauncherActionMontage"),
				LauncherData->AttackMontage,
				1.0f
			);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &UFTGA_LauncherAction::HandleMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_LauncherAction::HandleMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &UFTGA_LauncherAction::HandleMontageInterrupted);
			MontageTask->ReadyForActivation();
		}
		else
		{
			ExecuteFire();
			EndLauncherAbility(false);
		}
	}
	else
	{
		ExecuteFire();
		EndLauncherAbility(false);
	}
}

bool UFTGA_LauncherAction::FireProjectile()
{
	// ActiveLauncherData/ProjectileActorData에서 발사에 필요한 설정을 꺼낸다.
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

	// 1. 카메라/컨트롤러 기준 발사 방향을 구한다.
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

	// 2. 장착된 발사기 Mesh에서 Muzzle 소켓을 찾는다.
	//    소켓을 찾지 못하면 카메라 위치에서 발사하는 fallback을 쓴다.
	if (UMeshComponent* LauncherMesh = ResolveLauncherMesh(Avatar, LauncherData->MuzzleSocketName))
	{
		SpawnLocation = LauncherMesh->GetSocketLocation(LauncherData->MuzzleSocketName);
		SpawnRotation = ViewRotation;
	}

	const FVector FireDirection = SpawnRotation.Vector().GetSafeNormal();

	// 3. ProjectileActor를 스폰한다. 실제 비행/충돌 설정은 ProjectileActorDataAsset이 담당한다.
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

	// 4. ProjectileActor에 ProjectileActorDataAsset 자체를 전달해 메시/속도/충돌/GE 적용 정보를 초기화한다.
	Projectile->InitializeProjectile(
		ProjectileActorData,
		FireDirection,
		Avatar
	);

	return true;
}

bool UFTGA_LauncherAction::EnsureProjectileAbilityGranted()
{
	// 투사체가 맞았을 때 실행될 ProjectileAction은 ProjectileActorDataAsset의 UseAbility에 들어 있다.
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

	// 이미 같은 Ability 클래스가 있으면 다시 부여하지 않는다.
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

	// ProjectileActorData를 SourceObject로 넣어두면 나중에 어떤 투사체 Ability인지 추적하기 쉽다.
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

void UFTGA_LauncherAction::EndLauncherAbility(bool bWasCancelled)
{
	// 발사기 어빌리티는 한 번 발사하고 끝나는 구조라 종료 시 캐시를 비운다.
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

	// 발사기는 캐릭터에 붙어 있는 별도 Actor라고 보고, AttachedActor 안의 Mesh에서 소켓을 찾는다.
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

bool UFTGA_LauncherAction::ExecuteFire()
{
	if (bHasFiredInThisActivation)
	{
		return false;
	}
	bHasFiredInThisActivation = true;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}

	const FName ProjectileItemId = ProjectileActorData->ItemData.ItemId;
	UFTInventoryComponent* InventoryComponent = Avatar->FindComponentByClass<UFTInventoryComponent>();
	if (!InventoryComponent || ProjectileItemId.IsNone())
	{
		return false;
	}

	if (InventoryComponent->GetItemQuantity(ProjectileItemId) <= 0)
	{
		UE_LOG(LogFTItem, Warning, TEXT("LauncherAction failed: no projectile item for firing. ItemId=%s"),
			*ProjectileItemId.ToString());
		return false;
	}

	const bool bFired = FireProjectile();
	if (!bFired)
	{
		return false;
	}

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = Avatar;
	Payload.ItemId = ProjectileItemId;

	UGameplayMessageSubsystem::Get(Avatar).BroadcastMessage(
		TAG_FT_Event_ItemConsumed,
		Payload
	);

	return true;
}

void UFTGA_LauncherAction::HandleThrowReleaseEvent(FGameplayEventData Payload)
{
	ExecuteFire();
	EndLauncherAbility(false);
}

void UFTGA_LauncherAction::HandleMontageCompleted()
{
	if (!bHasFiredInThisActivation)
	{
		ExecuteFire();
	}
	EndLauncherAbility(false);
}

void UFTGA_LauncherAction::HandleMontageInterrupted()
{
	EndLauncherAbility(true);
}
