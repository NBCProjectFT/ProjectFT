
#include "FTGA_HitScanAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Components/MeshComponent.h"
#if ENABLE_DRAW_DEBUG
#include "KismetTraceUtils.h"
#endif
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTCrosshairComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTHitScanDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Struct/FTHitScanActionStruct.h"

UFTGA_HitScanAction::UFTGA_HitScanAction()
{
	// 발동 중인 아이템 데이터와 몽타주 상태를 인스턴스에 보관한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 아이템 사용 입력은 공통 Event.UseItem으로 들어온다.
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

	// Payload.OptionalObject에 들어온 ItemDataAsset을 히트스캔 전용 데이터로 캐싱한다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ActiveHitScanData = Cast<UFTHitScanDataAsset>(ActiveItemData);

	// 이 Ability는 UFTHitScanDataAsset 전용이다.
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

	// 실제 발사 판정 전에 비용/쿨다운을 확정한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndHitScanAbility(true);
		return;
	}

	// 히트스캔은 즉발 판정이므로 발동 즉시 Trace와 Effect 적용을 끝낸다.
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UFTCrosshairComponent* CrosshairComponent = Avatar->FindComponentByClass<UFTCrosshairComponent>())
		{
			CrosshairComponent->NotifyFired();
		}
	}

	PerformHitScan();

	if (HitScanData->AttackMontage)
	{
		// 현재는 몽타주 종료를 기다리지 않고 재생만 시킨 뒤 Ability를 종료한다.
		// 아래 AbilityTask 방식은 몽타주 수명까지 Ability를 유지하고 싶을 때 다시 사용할 수 있는 흔적이다.
		// UAbilityTask_PlayMontageAndWait* MontageTask =
		// 	UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		// 		this,
		// 		TEXT("HitScanActionMontage"),
		// 		HitScanData->AttackMontage,
		// 		1.0f
		// 	);
		//
		//
		// if (!MontageTask)
		// {
		// 	EndHitScanAbility(true);
		// 	return;
		// }
		//
		// MontageTask->OnCompleted.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageCompleted);
		// MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageInterrupted);
		// MontageTask->OnCancelled.AddDynamic(this, &UFTGA_HitScanAction::HandleMontageInterrupted);
		//
		// MontageTask->ReadyForActivation();
		// return;
		if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
		{
			AnimInstance->Montage_Play(HitScanData->AttackMontage, 1.0f);
		}
	}

	EndHitScanAbility(false);
}

void UFTGA_HitScanAction::PerformHitScan()
{
	// DataAsset에서 사거리, 트레이스 채널, 총구 소켓 같은 발사 설정을 읽는다.
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

	// 1. 카메라/컨트롤러 기준 조준 방향을 구한다.
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

	// 장착된 무기는 자기 자신과 충돌하면 안 되므로 Trace에서 제외한다.
	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor)
		{
			Params.AddIgnoredActor(AttachedActor);
		}
	}

	// 2. 화면 중앙 기준으로 먼저 조준점을 찾는다.
	//    총구에서 바로 쏘면 카메라 조준점과 어긋날 수 있어서 두 단계 Trace를 쓴다.
	FHitResult CameraHit;
	const bool bCameraHit = World->LineTraceSingleByChannel(
		CameraHit,
		CameraTraceStart,
		CameraTraceEnd,
		HitScanData->TraceChannel,
		Params
	);

	const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : CameraTraceEnd;

	// 3. 장착 무기에서 Muzzle 소켓을 찾아 실제 발사 시작점을 정한다.
	FVector MuzzleLocation = CameraTraceStart;

	if (UMeshComponent* WeaponMesh = ResolveWeaponMesh(Avatar, HitScanData->MuzzleSocketName))
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(HitScanData->MuzzleSocketName);
	}

	// 4. 총구에서 조준점을 향해 실제 판정 Trace를 수행한다.
	//    AimPoint에서 끝내면 적 표면 바로 앞에서 Trace가 끊길 수 있으므로, 방향만 AimPoint로 잡고 길이는 DataAsset의 Range를 사용한다.
	const FVector WeaponTraceDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	const FVector WeaponTraceEnd = MuzzleLocation + WeaponTraceDirection * HitScanData->Range;
	FHitResult WeaponHit;

	const bool bWeaponHit = World->LineTraceSingleByChannel(
		WeaponHit,
		MuzzleLocation,
		WeaponTraceEnd,
		HitScanData->TraceChannel,
		Params
	);

#if ENABLE_DRAW_DEBUG
	if (HitScanData->bDrawDebug)
	{
		// UE의 Blueprint LineTraceByChannel 디버그와 같은 유틸을 사용한다.
		// 빨강: TraceColor, 초록: TraceHitColor, 작은 점: 실제 ImpactPoint.

		DrawDebugLineTraceSingle(
			World,
			MuzzleLocation,
			WeaponTraceEnd,
			EDrawDebugTrace::ForDuration,
			bWeaponHit,
			WeaponHit,
			FLinearColor::Red,
			FLinearColor::Green,
			5.0f);
	}
#endif

	if (!bWeaponHit || !WeaponHit.GetActor())
	{
		UE_LOG(LogFTItem, Log, TEXT("HitScanAction missed."));
		return;
	}

	// 맞은 HitResult를 TargetData로 바꿔 베이스의 ApplyUseEffects 경로를 사용한다.
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
	// 다음 발동에 이전 아이템 데이터가 남지 않도록 정리한다.
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

	// 현재 구조에서는 무기가 캐릭터에 Actor로 Attach되어 있다고 보고, 그 자식 Actor들의 Mesh를 훑는다.
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
