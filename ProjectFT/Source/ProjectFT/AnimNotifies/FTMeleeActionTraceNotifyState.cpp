#include "FTMeleeActionTraceNotifyState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Player/FTPlayerCharacter.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"
#include "ProjectFT/Struct/FTMeleeActionStruct.h"

void UFTMeleeActionTraceNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	PreviousTraceFrames.Remove(MeshComp);
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

	PreviousTraceFrames.Remove(MeshComp);
	SendTraceEndEvent(MeshComp);
}

const FFTMeleeActionStruct* UFTMeleeActionTraceNotifyState::ResolveMeleeActionData(
	const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	const AFTPlayerCharacter* PlayerCharacter = Cast<AFTPlayerCharacter>(OwnerActor);
	if (!PlayerCharacter)
	{
		return nullptr;
	}

	const FFTInventoryItem& HeldItem = PlayerCharacter->GetCurrentHeldInventoryItem();

	UFTItemDataAsset* ItemDataAsset = HeldItem.ItemDataAsset.Get();
	if (!ItemDataAsset)
	{
		return nullptr;
	}

	const UFTMeleeDataAsset* MeleeDataAsset = Cast<UFTMeleeDataAsset>(ItemDataAsset);
	if (!MeleeDataAsset)
	{
		return nullptr;
	}

	return &MeleeDataAsset->MeleeActionData;
}

UMeshComponent* UFTMeleeActionTraceNotifyState::ResolveTraceMesh(
	const USkeletalMeshComponent* MeshComp,
	FName InHitStartSocketName,
	FName InHitEndSocketName) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

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

			if (MeshComponent->DoesSocketExist(InHitStartSocketName) &&
				MeshComponent->DoesSocketExist(InHitEndSocketName))
			{
				return MeshComponent;
			}
		}
	}

	// 2. fallback: 캐릭터 Mesh에 소켓이 있으면 캐릭터 Mesh 사용
	if (MeshComp->DoesSocketExist(InHitStartSocketName) &&
		MeshComp->DoesSocketExist(InHitEndSocketName))
	{
		return const_cast<USkeletalMeshComponent*>(MeshComp);
	}

	return nullptr;
}

bool UFTMeleeActionTraceNotifyState::BuildTraceCapsule(
	const UMeshComponent* TraceMesh,
	FName InHitStartSocketName,
	FName InHitEndSocketName,
	float InCapsuleRadius,
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

	if (!TraceMesh->DoesSocketExist(InHitStartSocketName) ||
		!TraceMesh->DoesSocketExist(InHitEndSocketName))
	{
		return false;
	}

	OutStart = TraceMesh->GetSocketLocation(InHitStartSocketName);
	OutEnd = TraceMesh->GetSocketLocation(InHitEndSocketName);

	const FVector Axis = OutEnd - OutStart;
	const float AxisLength = Axis.Size();

	if (AxisLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutHalfHeight = FMath::Max(AxisLength * 0.5f, InCapsuleRadius);
	OutCenter = (OutStart + OutEnd) * 0.5f;
	OutRotation = FRotationMatrix::MakeFromZ(Axis / AxisLength).ToQuat();

	return true;
}

void UFTMeleeActionTraceNotifyState::SendTraceBeginEvent(USkeletalMeshComponent* MeshComp) const
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
	EventData.EventTag = TAG_FT_Event_Melee_Begin;
	EventData.Instigator = OwnerActor;
	EventData.OptionalObject = MeshComp;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		TAG_FT_Event_Melee_Begin,
		EventData
	);
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

	const FFTMeleeActionStruct* MeleeData = ResolveMeleeActionData(MeshComp);

	// MeleeDataAsset이 있으면 DataAsset 값 사용, 없으면 NotifyState 기본값 사용
	const FName RuntimeHitStartSocketName =
		MeleeData ? MeleeData->HitStartSocketName : HitStartSocketName;

	const FName RuntimeHitEndSocketName =
		MeleeData ? MeleeData->HitEndSocketName : HitEndSocketName;

	const float RuntimeCapsuleRadius =
		MeleeData ? MeleeData->CapsuleRadius : CapsuleRadius;

	const ECollisionChannel RuntimeTraceChannel =
		MeleeData ? MeleeData->TraceChannel.GetValue() : TraceChannel.GetValue();

	const bool bRuntimeDrawDebug =
		MeleeData ? MeleeData->bDrawDebug : bDrawDebug;

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Center = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	float HalfHeight = 0.0f;

	UMeshComponent* TraceMesh = ResolveTraceMesh(
		MeshComp,
		RuntimeHitStartSocketName,
		RuntimeHitEndSocketName
	);

	if (!TraceMesh)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Melee trace failed: TraceMesh not found. StartSocket=%s EndSocket=%s"),
			*RuntimeHitStartSocketName.ToString(),
			*RuntimeHitEndSocketName.ToString());

		return;
	}

	if (!BuildTraceCapsule(
		TraceMesh,
		RuntimeHitStartSocketName,
		RuntimeHitEndSocketName,
		RuntimeCapsuleRadius,
		Start,
		End,
		Center,
		HalfHeight,
		Rotation))
	{
		UE_LOG(LogFTItem, Warning, TEXT("Melee trace failed: BuildTraceCapsule failed. Mesh=%s StartSocket=%s EndSocket=%s"),
			*GetNameSafe(TraceMesh),
			*RuntimeHitStartSocketName.ToString(),
			*RuntimeHitEndSocketName.ToString());

		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FTMeleeNotifyTrace), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	const FPreviousTraceFrame* PreviousFrame = PreviousTraceFrames.Find(MeshComp);
	const float MaxSocketMovement = PreviousFrame
		? FMath::Max(
			FVector::Distance(PreviousFrame->Start, Start),
			FVector::Distance(PreviousFrame->End, End))
		: 0.0f;

	// 캡슐 반지름보다 촘촘하게 중간 위치를 채워 빠른 휘두르기에서도 판정 사이가 비지 않게 한다.
	const float SampleSpacing = FMath::Max(RuntimeCapsuleRadius * 0.75f, 1.0f);
	const int32 SubstepCount = PreviousFrame
		? FMath::Clamp(FMath::CeilToInt(MaxSocketMovement / SampleSpacing), 1, 32)
		: 1;

	// 보간 캡슐 여러 개가 같은 액터를 찾아도 한 Tick에는 Hit 이벤트를 한 번만 보낸다.
	TSet<AActor*> ReportedActors;

	for (int32 SubstepIndex = 1; SubstepIndex <= SubstepCount; ++SubstepIndex)
	{
		const float Alpha = static_cast<float>(SubstepIndex) / static_cast<float>(SubstepCount);
		const FVector SampleStart = PreviousFrame
			? FMath::Lerp(PreviousFrame->Start, Start, Alpha)
			: Start;
		const FVector SampleEnd = PreviousFrame
			? FMath::Lerp(PreviousFrame->End, End, Alpha)
			: End;
		const FVector SampleAxis = SampleEnd - SampleStart;
		const float SampleAxisLength = SampleAxis.Size();

		if (SampleAxisLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector SampleCenter = (SampleStart + SampleEnd) * 0.5f;
		const float SampleHalfHeight = FMath::Max(SampleAxisLength * 0.5f, RuntimeCapsuleRadius);
		const FQuat SampleRotation = FRotationMatrix::MakeFromZ(SampleAxis / SampleAxisLength).ToQuat();

		TArray<FOverlapResult> OverlapResults;
		const bool bHit = World->OverlapMultiByChannel(
			OverlapResults,
			SampleCenter,
			SampleRotation,
			RuntimeTraceChannel,
			FCollisionShape::MakeCapsule(RuntimeCapsuleRadius, SampleHalfHeight),
			QueryParams);

		if (bRuntimeDrawDebug)
		{
			DrawDebugCapsule(
				World,
				SampleCenter,
				SampleHalfHeight,
				RuntimeCapsuleRadius,
				SampleRotation,
				FColor::Red,
				false,
				0.08f,
				0,
				1.5f);
		}

		if (!bHit)
		{
			continue;
		}

		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* HitActor = OverlapResult.GetActor();

			if (!HitActor || HitActor == OwnerActor || ReportedActors.Contains(HitActor))
			{
				continue;
			}

			ReportedActors.Add(HitActor);

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
				EventData);
		}
	}

	FPreviousTraceFrame& CurrentFrame = PreviousTraceFrames.FindOrAdd(MeshComp);
	CurrentFrame.Start = Start;
	CurrentFrame.End = End;
}
