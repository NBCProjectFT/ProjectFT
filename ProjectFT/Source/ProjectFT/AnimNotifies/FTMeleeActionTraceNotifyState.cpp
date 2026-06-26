
#include "FTMeleeActionTraceNotifyState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectFT/Core/FTLogChannels.h"

void UFTMeleeActionTraceNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	SendTraceBeginEvent(MeshComp);

	// Begin 프레임에서 바로 겹쳐 있는 적을 놓치지 않기 위해 한 번 검사
	TraceAndSendHitEvent(MeshComp);
}

void UFTMeleeActionTraceNotifyState::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	TraceAndSendHitEvent(MeshComp);
}

void UFTMeleeActionTraceNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	SendTraceEndEvent(MeshComp);
}

UMeshComponent* UFTMeleeActionTraceNotifyState::ResolveTraceMesh(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) return nullptr;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return nullptr;
		
	// 1. 캐릭터에 Attach된 액터들 중에서 Hit_Start / Hit_End 소켓 가진 Mesh 찾기
	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors);

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

			if (MeshComponent->DoesSocketExist(HitStartSocketName) &&
				MeshComponent->DoesSocketExist(HitEndSocketName))
			{
				return MeshComponent;
			}
		}
	}

	// 2. fallback: 캐릭터 Mesh에 소켓이 있으면 캐릭터 Mesh 사용
	if (MeshComp->DoesSocketExist(HitStartSocketName) &&
		MeshComp->DoesSocketExist(HitEndSocketName))
	{
		return const_cast<USkeletalMeshComponent*>(MeshComp);
	}

	return nullptr;
}

bool UFTMeleeActionTraceNotifyState::BuildTraceCapsule(
	const UMeshComponent* TraceMesh,
	FVector& OutStart,
	FVector& OutEnd,
	FVector& OutCenter,
	float& OutHalfHeight,
	FQuat& OutRotation) const
{
	if (!TraceMesh)
	{
		return false;
	}

	if (!TraceMesh->DoesSocketExist(HitStartSocketName) ||
		!TraceMesh->DoesSocketExist(HitEndSocketName))
	{
		return false;
	}

	OutStart = TraceMesh->GetSocketLocation(HitStartSocketName);
	OutEnd = TraceMesh->GetSocketLocation(HitEndSocketName);

	const FVector Axis = OutEnd - OutStart;
	const float AxisLength = Axis.Size();

	if (AxisLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutHalfHeight = FMath::Max(AxisLength * 0.5f, CapsuleRadius);
	OutCenter = (OutStart + OutEnd) * 0.5f;
	OutRotation = FRotationMatrix::MakeFromZ(Axis / AxisLength).ToQuat();

	return true;
}

void UFTMeleeActionTraceNotifyState::SendTraceBeginEvent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) return;
	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	FGameplayEventData EventData;
	EventData.EventTag = TAG_FT_Event_Melee_Begin;
	EventData.Instigator = OwnerActor;
	EventData.OptionalObject = MeshComp;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, TAG_FT_Event_Melee_Begin, EventData);
}

void UFTMeleeActionTraceNotifyState::SendTraceEndEvent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_FT_Event_Melee_End;
	EventData.Instigator = OwnerActor;
	EventData.OptionalObject = MeshComp;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		TAG_FT_Event_Melee_End,
		EventData
	);
}

void UFTMeleeActionTraceNotifyState::TraceAndSendHitEvent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!World)
	{
		return;
	}

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Center = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	float HalfHeight = 0.0f;

	UMeshComponent* TraceMesh = ResolveTraceMesh(MeshComp);

	if (!TraceMesh) return;
	if (!BuildTraceCapsule(TraceMesh, Start, End, Center, HalfHeight, Rotation)) return;
	

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FTMeleeNotifyTrace), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> OverlapResults;

	const bool bHit = World->OverlapMultiByChannel(
		OverlapResults,
		Center,
		Rotation,
		TraceChannel,
		FCollisionShape::MakeCapsule(CapsuleRadius, HalfHeight),
		QueryParams
	);

	if (bDrawDebug)
	{
		const float DebugLifeTime = 0.08f;
		const float DebugThickness = 1.5f;

		DrawDebugCapsule(World, Center, HalfHeight, CapsuleRadius, Rotation, FColor::Red, false, DebugLifeTime, 0, DebugThickness);
	}

	if (!bHit)
	{
		return;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* HitActor = OverlapResult.GetActor();

		if (!HitActor || HitActor == OwnerActor)
		{
			continue;
		}
		FGameplayEventData EventData;
		EventData.EventTag = TAG_FT_Event_Melee_Hit;
		EventData.Instigator = OwnerActor;
		EventData.Target = HitActor;
		EventData.OptionalObject = TraceMesh;
		EventData.TargetData =
			UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor);

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			OwnerActor,
			TAG_FT_Event_Melee_Hit,
			EventData
		);
	}
}