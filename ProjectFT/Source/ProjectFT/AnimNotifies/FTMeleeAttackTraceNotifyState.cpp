// Fill out your copyright notice in the Description page of Project Settings.

#include "FTMeleeAttackTraceNotifyState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_MeleeAttack.h"
#include "ProjectFT/Core/FTLogChannels.h"

void UFTMeleeAttackTraceNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UpdateTraceCollision(MeshComp, true);

	if (UFTGA_MeleeAttack* MeleeAttack = FindActiveMeleeAttackAbility(MeshComp))
	{
		MeleeAttack->StartMeleeTrace();
		return;
	}

	UE_LOG(LogFTItem, Warning, TEXT("Melee trace notify begin skipped: active MeleeAttack ability not found. Mesh=%s Animation=%s"),
		*GetNameSafe(MeshComp),
		*GetNameSafe(Animation));
}

void UFTMeleeAttackTraceNotifyState::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	UpdateTraceCollision(MeshComp, false);

	if (UFTGA_MeleeAttack* MeleeAttack = FindActiveMeleeAttackAbility(MeshComp))
	{
		MeleeAttack->PerformMeleeTrace();
	}
}

void UFTMeleeAttackTraceNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UFTGA_MeleeAttack* MeleeAttack = FindActiveMeleeAttackAbility(MeshComp))
	{
		MeleeAttack->StopMeleeTrace();
	}

	DestroyTraceCollision(MeshComp);
}

UFTGA_MeleeAttack* UFTMeleeAttackTraceNotifyState::FindActiveMeleeAttackAbility(const USkeletalMeshComponent* MeshComp) const
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return nullptr;
	}

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UFTGA_MeleeAttack* MeleeAttack = Cast<UFTGA_MeleeAttack>(AbilityInstance))
			{
				return MeleeAttack;
			}
		}
	}

	return nullptr;
}

void UFTMeleeAttackTraceNotifyState::UpdateTraceCollision(const USkeletalMeshComponent* MeshComp, bool bLogMissingSockets)
{
	if (!MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		return;
	}

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float CapsuleHalfHeight = 0.0f;
	FQuat CapsuleRotation = FQuat::Identity;
	if (!BuildTraceCapsule(MeshComp, Start, End, CapsuleHalfHeight, CapsuleRotation, bLogMissingSockets))
	{
		return;
	}

	const FVector CapsuleCenter = (Start + End) * 0.5f;

	if (bCreateTraceCollision)
	{
		if (UCapsuleComponent* TraceCapsule = FindOrCreateTraceCollision(MeshComp))
		{
			TraceCapsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight, true);
			TraceCapsule->SetWorldLocationAndRotation(CapsuleCenter, CapsuleRotation);
		}
	}

	if (!bDrawDebug)
	{
		return;
	}

	constexpr float DebugLifeTime = 0.08f;
	constexpr float DebugThickness = 2.0f;

	DrawDebugSphere(World, Start, CapsuleRadius, 12, FColor::Yellow, false, DebugLifeTime, 0, DebugThickness);
	DrawDebugSphere(World, End, CapsuleRadius, 12, FColor::Orange, false, DebugLifeTime, 0, DebugThickness);
	DrawDebugLine(World, Start, End, FColor::Yellow, false, DebugLifeTime, 0, DebugThickness);
	DrawDebugCapsule(World, CapsuleCenter, CapsuleHalfHeight, CapsuleRadius, CapsuleRotation, FColor::Red, false, DebugLifeTime, 0, DebugThickness);
}

void UFTMeleeAttackTraceNotifyState::DestroyTraceCollision(const USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return;
	}

	const uint32 MeshKey = MeshComp->GetUniqueID();
	TWeakObjectPtr<UCapsuleComponent> TraceCapsulePtr;
	if (!ActiveTraceCapsules.RemoveAndCopyValue(MeshKey, TraceCapsulePtr))
	{
		return;
	}

	if (UCapsuleComponent* TraceCapsule = TraceCapsulePtr.Get())
	{
		TraceCapsule->DestroyComponent();
	}
}

UCapsuleComponent* UFTMeleeAttackTraceNotifyState::FindOrCreateTraceCollision(const USkeletalMeshComponent* MeshComp)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return nullptr;
	}

	const uint32 MeshKey = MeshComp->GetUniqueID();
	if (TWeakObjectPtr<UCapsuleComponent>* ExistingCapsulePtr = ActiveTraceCapsules.Find(MeshKey))
	{
		if (UCapsuleComponent* ExistingCapsule = ExistingCapsulePtr->Get())
		{
			return ExistingCapsule;
		}
	}

	UCapsuleComponent* TraceCapsule = NewObject<UCapsuleComponent>(Owner, UCapsuleComponent::StaticClass(), TEXT("FT_MeleeTraceCapsule"));
	if (!TraceCapsule)
	{
		return nullptr;
	}

	TraceCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TraceCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	TraceCapsule->SetCollisionResponseToAllChannels(ECR_Overlap);
	TraceCapsule->SetGenerateOverlapEvents(true);
	TraceCapsule->SetHiddenInGame(false);
	TraceCapsule->SetVisibility(true);
	TraceCapsule->ShapeColor = FColor::Red;
	TraceCapsule->RegisterComponent();
	Owner->AddInstanceComponent(TraceCapsule);

	ActiveTraceCapsules.Add(MeshKey, TraceCapsule);
	return TraceCapsule;
}

const UMeshComponent* UFTMeleeAttackTraceNotifyState::ResolveTraceMesh(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	const AActor* Owner = MeshComp->GetOwner();
	const UMeshComponent* FirstAttachedMesh = nullptr;
	if (Owner)
	{
		TArray<AActor*> AttachedActors;
		Owner->GetAttachedActors(AttachedActors);
		for (const AActor* AttachedActor : AttachedActors)
		{
			if (!AttachedActor)
			{
				continue;
			}

			const UMeshComponent* AttachedMesh = AttachedActor->FindComponentByClass<UMeshComponent>();
			if (!AttachedMesh)
			{
				continue;
			}

			if (!FirstAttachedMesh)
			{
				FirstAttachedMesh = AttachedMesh;
			}

			if (AttachedMesh->DoesSocketExist(HitStartSocketName) && AttachedMesh->DoesSocketExist(HitEndSocketName))
			{
				return AttachedMesh;
			}
		}
	}

	if (MeshComp->DoesSocketExist(HitStartSocketName) && MeshComp->DoesSocketExist(HitEndSocketName))
	{
		return MeshComp;
	}

	return FirstAttachedMesh ? FirstAttachedMesh : MeshComp;
}

bool UFTMeleeAttackTraceNotifyState::BuildTraceCapsule(
	const USkeletalMeshComponent* MeshComp,
	FVector& OutStart,
	FVector& OutEnd,
	float& OutHalfHeight,
	FQuat& OutRotation,
	bool bLogMissingSockets) const
{
	if (!MeshComp)
	{
		return false;
	}

	const UMeshComponent* TraceMesh = ResolveTraceMesh(MeshComp);
	if (!TraceMesh)
	{
		return false;
	}

	const bool bHasStartSocket = TraceMesh->DoesSocketExist(HitStartSocketName);
	const bool bHasEndSocket = TraceMesh->DoesSocketExist(HitEndSocketName);
	if (bHasStartSocket && bHasEndSocket)
	{
		OutStart = TraceMesh->GetSocketLocation(HitStartSocketName);
		OutEnd = TraceMesh->GetSocketLocation(HitEndSocketName);
	}
	else if (bDrawFallbackWhenSocketsMissing)
	{
		if (bLogMissingSockets)
		{
			UE_LOG(LogFTItem, Warning, TEXT("Melee notify debug using mesh-follow fallback capsule: trace mesh=%s Start=%s(%s) End=%s(%s)"),
				*GetNameSafe(TraceMesh),
				*HitStartSocketName.ToString(),
				bHasStartSocket ? TEXT("Found") : TEXT("Missing"),
				*HitEndSocketName.ToString(),
				bHasEndSocket ? TEXT("Found") : TEXT("Missing"));
		}

		const FVector Axis = TraceMesh->GetForwardVector();
		const FVector Origin = TraceMesh->GetComponentLocation();
		OutStart = Origin - Axis * (FallbackCapsuleLength * 0.5f);
		OutEnd = Origin + Axis * (FallbackCapsuleLength * 0.5f);
	}
	else
	{
		if (bLogMissingSockets)
		{
			UE_LOG(LogFTItem, Warning, TEXT("Melee notify debug skipped: sockets missing. Mesh=%s Start=%s(%s) End=%s(%s)"),
				*GetNameSafe(TraceMesh),
				*HitStartSocketName.ToString(),
				bHasStartSocket ? TEXT("Found") : TEXT("Missing"),
				*HitEndSocketName.ToString(),
				bHasEndSocket ? TEXT("Found") : TEXT("Missing"));
		}
		return false;
	}

	const FVector CapsuleAxis = OutEnd - OutStart;
	if (CapsuleAxis.IsNearlyZero())
	{
		return false;
	}

	OutHalfHeight = FMath::Max(CapsuleAxis.Size() * 0.5f, CapsuleRadius);
	OutRotation = FRotationMatrix::MakeFromZ(CapsuleAxis.GetSafeNormal()).ToQuat();
	return true;
}
