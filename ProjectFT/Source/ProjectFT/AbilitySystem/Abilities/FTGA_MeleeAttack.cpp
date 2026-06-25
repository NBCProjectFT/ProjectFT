// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"

UFTGA_MeleeAttack::UFTGA_MeleeAttack()
{
	// QuickSlot에서 ItemData payload를 넘긴 뒤, Montage/Notify 구간 동안 상태를 들고 있어야 하므로 인스턴스형 Ability로 사용합니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActiveItemData = TriggerEventData ? const_cast<UFTItemDataAsset*>(Cast<UFTItemDataAsset>(TriggerEventData->OptionalObject)) : nullptr;
	ActiveMeleeData = Cast<UFTMeleeDataAsset>(ActiveItemData);
	HitActors.Reset();
	bTraceActive = false;

	UE_LOG(LogFTItem, Log, TEXT("MeleeAttack activated. Avatar=%s Item=%s MeleeData=%s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(ActiveItemData),
		*GetNameSafe(ActiveMeleeData));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();
	if (!MeleeData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack ended: ItemData is not UFTMeleeDataAsset. Ability=%s Item=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveItemData));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!MeleeData->AttackMontage)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack ended: AttackMontage is not set. Ability=%s Item=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveItemData));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("MeleeAttackMontage"),
		MeleeData->AttackMontage,
		1.0f);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UFTGA_MeleeAttack::StartMeleeTrace()
{
	HitActors.Reset();
	bTraceActive = true;

	// NotifyBegin이 들어온 바로 그 프레임도 놓치지 않게 한 번 검사합니다.
	PerformMeleeTrace();
}

void UFTGA_MeleeAttack::PerformMeleeTrace()
{
	if (!bTraceActive)
	{
		return;
	}

	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();
	if (!MeleeData)
	{
		return;
	}

	USkeletalMeshComponent* SourceMesh = ResolveSourceMesh();
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceMesh || !SourceActor)
	{
		return;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		return;
	}

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float CapsuleHalfHeight = 0.0f;
	FQuat CapsuleRotation = FQuat::Identity;
	if (!BuildTraceCapsule(SourceMesh, Start, End, CapsuleHalfHeight, CapsuleRotation))
	{
		return;
	}

	const FVector CapsuleCenter = (Start + End) * 0.5f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeAttack), false, SourceActor);
	QueryParams.AddIgnoredActor(SourceActor);

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(
		OverlapResults,
		CapsuleCenter,
		CapsuleRotation,
		MeleeData->TraceChannel,
		FCollisionShape::MakeCapsule(MeleeData->CapsuleRadius, CapsuleHalfHeight),
		QueryParams);

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		if (!TargetActor || TargetActor == SourceActor || HitActors.Contains(TargetActor))
		{
			continue;
		}

		HitActors.Add(TargetActor);
		ApplyItemEffectsToTarget(TargetActor);
	}

	if (MeleeData->bDrawDebug)
	{
		constexpr float DebugLifeTime = 0.12f;
		constexpr float DebugThickness = 2.0f;
		DrawDebugSphere(World, Start, MeleeData->CapsuleRadius, 12, FColor::Yellow, false, DebugLifeTime, 0, DebugThickness);
		DrawDebugSphere(World, End, MeleeData->CapsuleRadius, 12, FColor::Orange, false, DebugLifeTime, 0, DebugThickness);
		DrawDebugLine(World, Start, End, FColor::Yellow, false, DebugLifeTime, 0, DebugThickness);
		DrawDebugCapsule(World, CapsuleCenter, CapsuleHalfHeight, MeleeData->CapsuleRadius, CapsuleRotation, FColor::Red, false, DebugLifeTime, 0, DebugThickness);
	}
}

void UFTGA_MeleeAttack::StopMeleeTrace()
{
	bTraceActive = false;
	HitActors.Reset();
}

void UFTGA_MeleeAttack::HandleMontageCompleted()
{
	EndMeleeAbility(false);
}

void UFTGA_MeleeAttack::HandleMontageInterrupted()
{
	EndMeleeAbility(true);
}

void UFTGA_MeleeAttack::EndMeleeAbility(bool bWasCancelled)
{
	StopMeleeTrace();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

const FFTMeleeAttackStruct* UFTGA_MeleeAttack::GetMeleeAttackData() const
{
	return ActiveMeleeData ? &ActiveMeleeData->MeleeAttackData : nullptr;
}

USkeletalMeshComponent* UFTGA_MeleeAttack::ResolveSourceMesh() const
{
	if (CurrentActorInfo && CurrentActorInfo->SkeletalMeshComponent.IsValid())
	{
		return CurrentActorInfo->SkeletalMeshComponent.Get();
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

bool UFTGA_MeleeAttack::BuildTraceCapsule(
	USkeletalMeshComponent* SourceMesh,
	FVector& OutStart,
	FVector& OutEnd,
	float& OutHalfHeight,
	FQuat& OutRotation) const
{
	if (!SourceMesh)
	{
		return false;
	}

	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();
	if (!MeleeData)
	{
		return false;
	}

	if (!SourceMesh->DoesSocketExist(MeleeData->HitStartSocketName) || !SourceMesh->DoesSocketExist(MeleeData->HitEndSocketName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack trace skipped: socket missing. Mesh=%s Start=%s(%s) End=%s(%s)"),
			*GetNameSafe(SourceMesh),
			*MeleeData->HitStartSocketName.ToString(),
			SourceMesh->DoesSocketExist(MeleeData->HitStartSocketName) ? TEXT("Found") : TEXT("Missing"),
			*MeleeData->HitEndSocketName.ToString(),
			SourceMesh->DoesSocketExist(MeleeData->HitEndSocketName) ? TEXT("Found") : TEXT("Missing"));
		return false;
	}

	OutStart = SourceMesh->GetSocketLocation(MeleeData->HitStartSocketName);
	OutEnd = SourceMesh->GetSocketLocation(MeleeData->HitEndSocketName);

	const FVector CapsuleAxis = OutEnd - OutStart;
	if (CapsuleAxis.IsNearlyZero())
	{
		return false;
	}

	// UE Capsule의 HalfHeight는 캡슐 전체 반높이입니다.
	// 여기서는 두 소켓 사이 거리를 캡슐의 길이 기준으로 삼고, 반지름보다 짧아지지 않게 보정합니다.
	OutHalfHeight = FMath::Max(CapsuleAxis.Size() * 0.5f, MeleeData->CapsuleRadius);
	OutRotation = FRotationMatrix::MakeFromZ(CapsuleAxis.GetSafeNormal()).ToQuat();
	return true;
}

void UFTGA_MeleeAttack::ApplyItemEffectsToTarget(AActor* TargetActor) const
{
	if (!ActiveItemData || !TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack effect skipped: ASC missing. SourceASC=%s Target=%s TargetASC=%s"),
			SourceASC ? TEXT("Valid") : TEXT("Missing"),
			*GetNameSafe(TargetActor),
			TargetASC ? TEXT("Valid") : TEXT("Missing"));
		return;
	}

	const FTItemUseStruct& UseData = ActiveItemData->ItemData.UseData;
	if (UseData.UseEffects.IsEmpty())
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack effect skipped: UseEffects is empty. Item=%s Target=%s"),
			*GetNameSafe(ActiveItemData),
			*GetNameSafe(TargetActor));
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(ActiveItemData);
	if (AActor* SourceActor = GetAvatarActorFromActorInfo())
	{
		EffectContext.AddInstigator(SourceActor, SourceActor);
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : UseData.UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle EffectSpec = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);
		if (!EffectSpec.IsValid())
		{
			continue;
		}

		for (const TPair<FGameplayTag, float>& Magnitude : UseData.EffectMagnitudes)
		{
			EffectSpec.Data->SetSetByCallerMagnitude(Magnitude.Key, Magnitude.Value);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), TargetASC);
		UE_LOG(LogFTItem, Log, TEXT("MeleeAttack effect applied. Item=%s Target=%s Effect=%s"),
			*GetNameSafe(ActiveItemData),
			*GetNameSafe(TargetActor),
			*GetNameSafe(EffectClass));
	}
}
