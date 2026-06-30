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
	// 입력 Released 대기, 몽타주 Notify 대기, HeldProjectile 상태를 인스턴스에 보관한다.
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
	
	// 투척 아이템도 일반 아이템 사용 입력(Event.UseItem)으로 시작한다.
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

	// Payload.OptionalObject에 들어온 아이템을 Throw DataAsset으로 캐싱한다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveThrowData = Cast<UFTThrowDataAsset>(ActiveItemData);

	// 이 Ability는 UFTThrowDataAsset 전용이다.
	if (!ActiveItemData || !ActiveThrowData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: invalid throw item data."));
		EndThrowAbility(true);
		return;
	}

	// Throw DataAsset은 실제로 손에 들고 발사할 ProjectileActorDataAsset을 참조한다.
	ProjectileActorData = ActiveThrowData->ThrowActorData.ProjectileItemData;

	if (!ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: ProjectileItemData is null."));
		EndThrowAbility(true);
		return;
	}

	const FFTThrowActorStruct* ThrowData = GetThrowActorData();
	const FFTProjectileActorStruct* ProjectileData = GetProjectileActorData();

	// ThrowData는 준비/투척 몽타주와 손 소켓, ProjectileData는 실제 스폰 클래스/비행 설정을 제공한다.
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

	// 발사체가 충돌했을 때 TargetHit 이벤트를 처리할 ProjectileAction 어빌리티를 보장한다.
	if (!EnsureProjectileAbilityGranted())
	{
		UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: EnsureProjectileAbilityGranted failed."));
		EndThrowAbility(true);
		return;
	}

	// 첫 발동 단계: 투사체를 손에 들고, 입력 Released를 기다리는 준비 상태로 들어간다.
	if (!bIsHoldingProjectile)
	{
		if (!StartHoldingProjectile())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: StartHoldingProjectile failed."));
			EndThrowAbility(true);
			return;
		}

		// 준비 몽타주는 선택 사항이다. 없어도 손에 들고 Released 대기는 가능하다.
		if (ThrowData->PrepareMontage)
		{
			if (UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Play(ThrowData->PrepareMontage, 1.0f);
			}
		}

		// 사용 입력을 놓을 때까지 Ability를 유지한다.
		if (!WaitForUseReleased())
		{
			UE_LOG(LogFTItem, Warning, TEXT("ThrowItemAction failed: WaitForUseReleased failed."));
			EndThrowAbility(true);
		}

		return;
	}

	// 이미 손에 든 상태에서 다시 들어온 경우에는 바로 Released 처리 경로로 보낸다.
	HandleUseReleasedEvent(FGameplayEventData());
}

void UFTGA_ThrowItemAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 외부 CancelAbilities 등 어떤 경로로 끝나도 손에 붙은 임시 투사체는 남기지 않는다.
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
	// 입력을 놓았으니 준비 상태에서 실제 투척 단계로 넘어간다.
	const FFTThrowActorStruct* ThrowData = GetThrowActorData();
	if (!ThrowData)
	{
		EndThrowAbility(true);
		return;
	}

	if (ThrowData->ThrowMontage)
	{
		// ThrowMontage가 있으면 Notify가 실제 발사 프레임을 알려줄 때까지 기다린다.
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
	// 몽타주 Notify가 실제 손을 놓는 프레임을 알려준 상태다.
	bWaitingForThrowRelease = false;
	ActiveThrowMontage = nullptr;

	if (!ReleaseHeldProjectile())
	{
		EndThrowAbility(true);
		return;
	}

	EndThrowAbility(false);
}

void UFTGA_ThrowItemAction::HandleThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Notify가 오기 전에 몽타주가 끝났다면 투척 타이밍을 놓친 것이므로 취소 처리한다.
	if (!bWaitingForThrowRelease || Montage != ActiveThrowMontage)
	{
		return;
	}

	EndThrowAbility(true);
}

bool UFTGA_ThrowItemAction::StartHoldingProjectile()
{
	// 준비 상태에서 손에 보여줄 ProjectileActor를 만든다.
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

	// 손에 붙일 Mesh는 캐릭터 본체 또는 장착 액터 중 AttachSocketName을 가진 Mesh다.
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

	// 손에 붙일 비주얼이므로 충돌 때문에 스폰 실패하지 않게 AlwaysSpawn을 사용한다.
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

	// Held 상태에서는 충돌/움직임을 꺼두고 외형만 보이게 초기화한다.
	HeldProjectile->InitializeHeldProjectile(
		ProjectileActorData,
		Avatar
	);

	// 소켓 위치/회전에 스냅하되 스케일은 부모 소켓 스케일을 강제로 따라가지 않는다.
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
	// PlayerCharacter::HandleUseItemReleased가 보내는 Event.UseReleased를 한 번만 기다린다.
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
	// UFTThrowReleaseAnimNotify가 보내는 Event.ThrowRelease를 한 번만 기다린다.
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
	// 실제 발사 단계. 여기서 비용/쿨다운을 커밋하고 아이템 소비를 알린다.
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

	// 손 소켓에서 분리한 뒤 현재 월드 위치를 유지한 채 ProjectileMovement를 시작한다.
	HeldProjectile->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform
	);

	HeldProjectile->ReleaseProjectile(ThrowDirection);

	// 인벤토리 차감은 메시지를 통해 InventoryComponent가 처리한다.
	OnItemConsumed();

	HeldProjectile = nullptr;
	bIsHoldingProjectile = false;
	bWaitingForThrowRelease = false;
	ActiveThrowMontage = nullptr;

	return true;
}

bool UFTGA_ThrowItemAction::EnsureProjectileAbilityGranted()
{
	// 투사체 충돌 후 실행될 ProjectileAction은 ProjectileActorDataAsset의 UseAbility에 들어 있다.
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

	UE_LOG(LogFTItem, Warning, TEXT("[ProjectileDebug] EnsureProjectileAbilityGranted. ProjectileData=%s Ability=%s"),
		*GetNameSafe(ProjectileActorData),
		*GetNameSafe(ProjectileAbilityClass.Get()));

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		UE_LOG(LogFTItem, Warning, TEXT("[ProjectileDebug] EnsureProjectileAbilityGranted failed: ASC is null."));
		return false;
	}

	// 같은 Ability 클래스가 이미 있으면 중복 부여하지 않는다.
	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		if (AbilitySpec.Ability->GetClass() == ProjectileAbilityClass)
		{
			UE_LOG(LogFTItem, Warning, TEXT("[ProjectileDebug] Projectile ability already granted. Ability=%s"),
				*GetNameSafe(ProjectileAbilityClass.Get()));
			return true;
		}
	}

	if (CurrentActorInfo && !CurrentActorInfo->IsNetAuthority())
	{
		return true;
	}

	// ProjectileActorData를 SourceObject로 넣어 어떤 투사체용 Ability인지 추적 가능하게 한다.
	FGameplayAbilitySpec AbilitySpec(
		ProjectileAbilityClass,
		1,
		INDEX_NONE,
		ProjectileActorData
	);

	ASC->GiveAbility(AbilitySpec);

	UE_LOG(LogFTItem, Warning, TEXT("[ProjectileDebug] Projectile ability granted now. Ability=%s SourceObject=%s"),
		*GetNameSafe(ProjectileAbilityClass.Get()),
		*GetNameSafe(ProjectileActorData));

	return true;
}

void UFTGA_ThrowItemAction::EndThrowAbility(bool bWasCancelled)
{
	// 취소 종료라면 아직 손에 붙어 있는 ProjectileActor를 제거한다.
	if (bWasCancelled && HeldProjectile)
	{
		ClearHeldProjectile();
	}

	if (!bIsHoldingProjectile)
	{
		// 손에 든 상태가 아니라면 다음 발동을 위해 활성 데이터 캐시를 비운다.
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
	// 준비 중 취소되었거나 강제 종료되었을 때 손에 남은 임시 액터를 제거한다.
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

	// 먼저 캐릭터 본체 Mesh에서 손 소켓을 찾는다.
	TArray<UMeshComponent*> AvatarMeshes;
	Avatar->GetComponents<UMeshComponent>(AvatarMeshes);

	for (UMeshComponent* MeshComponent : AvatarMeshes)
	{
		if (MeshComponent && MeshComponent->DoesSocketExist(RequiredSocketName))
		{
			return MeshComponent;
		}
	}

	// 없으면 캐릭터에 붙어 있는 장착 액터들의 Mesh에서 찾는다.
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
	// 플레이어가 바라보는 방향을 투척 방향으로 사용한다. 컨트롤러가 없으면 Actor Eyes 방향으로 fallback한다.
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
