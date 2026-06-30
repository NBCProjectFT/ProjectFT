
#include "FTGA_HitScanAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Components/MeshComponent.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTHitScanDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Struct/FTHitScanActionStruct.h"

UFTGA_HitScanAction::UFTGA_HitScanAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_HitScanAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveHitScanData = Cast<UFTHitScanDataAsset>(ActiveItemData);

	if (!ActiveItemData || !ActiveHitScanData)
	{
		EndHitScanAbility(true);
		return;
	}
	
	const FFTHitScanActionStruct* HitScanData = GetHitScanActionData();
	if (!HitScanData)
	{
		EndHitScanAbility(true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndHitScanAbility(true);
		return;
	}

	PerformHitScan();

	if (HitScanData->AttackMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				TEXT("HitScanActionMontage"),
				HitScanData->AttackMontage,
				1.0f
			);

		
		if (!MontageTask)
		{
			EndHitScanAbility(true);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageInterrupted);

		MontageTask->ReadyForActivation();
		return;
	}

	EndHitScanAbility(false);
}

void UFTGA_HitScanAction::PerformHitScan()
{
	const FFTHitScanActionStruct* HitScanData = GetHitScanActionData();
	if (!HitScanData)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. 화면 중앙 방향 구하기
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

	const FVector CameraTraceStart = ViewLocation;
	const FVector CameraTraceEnd =
		CameraTraceStart + ViewRotation.Vector() * HitScanData->Range;

	FCollisionQueryParams Params(FName(TEXT("FTHitScanAction")), false, Avatar);
	Params.AddIgnoredActor(Avatar);

	// 장착된 무기는 자기 자신이므로 무시
	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor)
		{
			Params.AddIgnoredActor(AttachedActor);
		}
	}

	// 2. 화면 중앙 기준으로 조준점 찾기
	FHitResult CameraHit;
	const bool bCameraHit = World->LineTraceSingleByChannel(
		CameraHit,
		CameraTraceStart,
		CameraTraceEnd,
		HitScanData->TraceChannel,
		Params
	);

	const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : CameraTraceEnd;

	// 3. 총구 위치 찾기
	FVector MuzzleLocation = CameraTraceStart;

	if (UMeshComponent* WeaponMesh = ResolveWeaponMesh(Avatar, HitScanData->MuzzleSocketName))
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(HitScanData->MuzzleSocketName);
	}

	// 4. 총구에서 조준점까지 실제 Trace
	FHitResult WeaponHit;

	const bool bWeaponHit = World->LineTraceSingleByChannel(
		WeaponHit,
		MuzzleLocation,
		AimPoint,
		HitScanData->TraceChannel,
		Params
	);

	if (HitScanData->bDrawDebug)
	{
		DrawDebugLine(
			World,
			MuzzleLocation,
			AimPoint,
			FColor::Red,
			false,
			1.0f,
			0,
			1.5f
		);
	}

	if (!bWeaponHit || !WeaponHit.GetActor())
	{
		UE_LOG(LogFTItem, Log, TEXT("HitScanAction missed."));
		return;
	}

	FGameplayAbilityTargetDataHandle TargetData =
		UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(WeaponHit);

	ApplyUseEffects(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		&TargetData
	);

	UE_LOG(LogFTItem, Log, TEXT("HitScanAction hit. Target=%s Bone=%s"),
		*GetNameSafe(WeaponHit.GetActor()),
		*WeaponHit.BoneName.ToString());
}

void UFTGA_HitScanAction::EndHitScanAbility(bool bWasCancelled)
{
	ActiveItemData = nullptr;
	ActiveHitScanData = nullptr;

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled);
}

const FFTHitScanActionStruct* UFTGA_HitScanAction::GetHitScanActionData() const
{
	return ActiveHitScanData ? &ActiveHitScanData->HitScanActionData : nullptr;
}

UMeshComponent* UFTGA_HitScanAction::ResolveWeaponMesh(AActor* Avatar, FName RequiredSocketName) const
{
	if (!Avatar) return nullptr;
	if (RequiredSocketName.IsNone()) return nullptr;

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

void UFTGA_HitScanAction::HandleMontageCompleted()
{
	EndHitScanAbility(false);
}

void UFTGA_HitScanAction::HandleMontageInterrupted()
{
	EndHitScanAbility(true);
}
