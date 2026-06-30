// Fill out your copyright notice in the Description page of Project Settings.

#include "FTTraversalComponent.h"

#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

UFTTraversalComponent::UFTTraversalComponent()
{
	// 평소엔 Tick하지 않고, 몽타주 없는 위치 보간 중에만 Tick을 켠다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFTTraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	// MotionWarpingComponent가 없으면 런타임에 붙인다(드롭인 편의). 워프는 이 컴포넌트가 전적으로 사용한다.
	if (OwnerCharacter)
	{
		MotionWarping = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();
		if (!MotionWarping)
		{
			MotionWarping = NewObject<UMotionWarpingComponent>(OwnerCharacter, TEXT("MotionWarping"));
			MotionWarping->RegisterComponent();
		}
	}
}

float UFTTraversalComponent::GetForwardTraceDistance() const
{
	const FVector LocalVelocity = OwnerCharacter->GetActorRotation().UnrotateVector(OwnerCharacter->GetVelocity());
	return FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, 500.0f),
		FVector2D(Settings.MinForwardTraceDistance, Settings.MaxForwardTraceDistance),
		LocalVelocity.X);
}

bool UFTTraversalComponent::FindTraversalCandidate(FFTTraversalCandidate& Out) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	const float R = Capsule->GetScaledCapsuleRadius();
	const float HH = Capsule->GetScaledCapsuleHalfHeight();
	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector FootLocation = ActorLocation - FVector(0.0f, 0.0f, HH);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	// ── 1) 전방 스윕으로 장애물 앞면 탐지 ──
	FHitResult ForwardHit;
	const bool bForwardHit = World->SweepSingleByChannel(
		ForwardHit, ActorLocation, ActorLocation + Forward * GetForwardTraceDistance(), FQuat::Identity,
		TraceChannel, FCollisionShape::MakeCapsule(30.0f, 60.0f), Params);

	if (!bForwardHit || !ForwardHit.bBlockingHit || !ForwardHit.GetComponent())
	{
		return false;
	}

	Out.HitComponent = ForwardHit.GetComponent();

	FVector FrontNormal = ForwardHit.ImpactNormal.GetSafeNormal2D(); // 수평으로 평탄화한 앞면 법선(캐릭터 쪽)
	if (FrontNormal.IsNearlyZero())
	{
		FrontNormal = -Forward;
	}
	const FVector IntoDirection = -FrontNormal; // 장애물 내부 방향(수평)

	// ── 2) 앞 윗모서리(FrontLedge): 앞면 약간 안쪽에서 아래로 트레이스해 윗면 Z를 찾는다 ──
	const FVector TopProbeXY = ForwardHit.ImpactPoint + IntoDirection * 2.0f;
	const FVector TopStart(TopProbeXY.X, TopProbeXY.Y, FootLocation.Z + Settings.MaxMantleHeight + 20.0f);
	const FVector TopEnd(TopProbeXY.X, TopProbeXY.Y, FootLocation.Z - 10.0f);
	FHitResult TopHit;
	if (!World->LineTraceSingleByChannel(TopHit, TopStart, TopEnd, TraceChannel, Params))
	{
		return false; // 윗면을 못 찾음 → 그냥 벽
	}

	Out.FrontLedgeLocation = FVector(ForwardHit.ImpactPoint.X, ForwardHit.ImpactPoint.Y, TopHit.ImpactPoint.Z);
	Out.FrontLedgeNormal = FrontNormal;
	Out.bHasFrontLedge = true;
	Out.ObstacleHeight = FMath::Abs(Out.FrontLedgeLocation.Z - FootLocation.Z);
	Out.DistanceToLedge = FVector::Dist2D(ActorLocation, Out.FrontLedgeLocation);

	// ── 3) 렛지 위에 캐릭터가 들어갈 공간이 있는지(천장/협소 차단) ──
	const FVector RoomFront = Out.FrontLedgeLocation + FrontNormal * (R + 2.0f) + FVector(0.0f, 0.0f, HH + 2.0f);
	FHitResult RoomHit;
	if (World->SweepSingleByChannel(RoomHit, ActorLocation, RoomFront, FQuat::Identity,
		TraceChannel, FCollisionShape::MakeCapsule(R, HH), Params))
	{
		return false;
	}

	// ── 4) 윗면을 가로질러 뒤 모서리(BackLedge)와 깊이(ObstacleDepth)를 찾는다 ──
	const float TopZ = Out.FrontLedgeLocation.Z;
	const int32 MaxSteps = FMath::Max(1, FMath::CeilToInt(Settings.MaxScanDepth / FMath::Max(1.0f, Settings.TopScanStep)));
	FVector LastTopPoint = Out.FrontLedgeLocation;
	bool bFoundBackEdge = false;

	for (int32 Step = 1; Step <= MaxSteps; ++Step)
	{
		const FVector ProbeXY = Out.FrontLedgeLocation + IntoDirection * (Settings.TopScanStep * Step);
		const FVector ProbeStart(ProbeXY.X, ProbeXY.Y, TopZ + 15.0f);
		const FVector ProbeEnd(ProbeXY.X, ProbeXY.Y, TopZ - 15.0f);
		FHitResult StepHit;
		const bool bOnTop = World->LineTraceSingleByChannel(StepHit, ProbeStart, ProbeEnd, TraceChannel, Params);

		if (bOnTop && FMath::Abs(StepHit.ImpactPoint.Z - TopZ) <= Settings.TopZTolerance)
		{
			LastTopPoint = FVector(ProbeXY.X, ProbeXY.Y, StepHit.ImpactPoint.Z);
			continue;
		}

		bFoundBackEdge = true; // 윗면이 끝남 = 뒤 모서리 통과
		break;
	}

	if (bFoundBackEdge)
	{
		Out.BackLedgeLocation = LastTopPoint;
		Out.BackLedgeNormal = IntoDirection;
		Out.bHasBackLedge = true;
		Out.ObstacleDepth = FVector::Dist2D(Out.FrontLedgeLocation, LastTopPoint);

		// ── 5) 뒤 모서리 너머로 내려 바닥을 찾는다(Hurdle vs Vault 구분) ──
		const FVector PastBack = LastTopPoint + IntoDirection * (R + 2.0f);
		const FVector FloorStart(PastBack.X, PastBack.Y, LastTopPoint.Z);
		const FVector FloorEnd = FloorStart - FVector(0.0f, 0.0f, Out.ObstacleHeight + 50.0f);
		FHitResult FloorHit;
		if (World->LineTraceSingleByChannel(FloorHit, FloorStart, FloorEnd, TraceChannel, Params))
		{
			Out.bHasFloor = true;
			Out.FloorLocation = FloorHit.ImpactPoint;
			Out.BackLedgeHeight = FMath::Abs(LastTopPoint.Z - FloorHit.ImpactPoint.Z);
		}
	}
	else
	{
		// 스캔 범위 내에서 윗면 끝이 안 보임 → 깊은 면 → Mantle 처리.
		Out.bHasBackLedge = false;
		Out.ObstacleDepth = Settings.MaxScanDepth;
	}

	if (bDrawDebug)
	{
		DrawDebugSphere(World, Out.FrontLedgeLocation, 8.0f, 12, FColor::Green, false, 2.0f);
		if (Out.bHasBackLedge) { DrawDebugSphere(World, Out.BackLedgeLocation, 8.0f, 12, FColor::Blue, false, 2.0f); }
		if (Out.bHasFloor) { DrawDebugSphere(World, Out.FloorLocation, 8.0f, 12, FColor::Yellow, false, 2.0f); }
	}

	return true;
}

void UFTTraversalComponent::ClassifyTraversal(FFTTraversalCandidate& C) const
{
	const bool bThin = (C.ObstacleDepth < Settings.ThinDepthThreshold);
	const bool bHeightForVault = (C.ObstacleHeight >= Settings.MinObstacleHeight && C.ObstacleHeight <= Settings.MaxVaultHeight);
	const bool bHeightForHurdle = (C.ObstacleHeight >= Settings.MinObstacleHeight && C.ObstacleHeight <= Settings.MaxHurdleHeight);
	const bool bHeightForMantle = (C.ObstacleHeight >= Settings.MinObstacleHeight && C.ObstacleHeight <= Settings.MaxMantleHeight);

	if (Settings.bAllowVault && C.bHasFrontLedge && C.bHasBackLedge && !C.bHasFloor && bThin && bHeightForVault)
	{
		C.ActionType = EFTTraversalActionType::Vault;
	}
	else if (Settings.bAllowHurdle && C.bHasFrontLedge && C.bHasBackLedge && C.bHasFloor && bThin && bHeightForHurdle
		&& C.BackLedgeHeight > Settings.MinBackFloorDrop)
	{
		C.ActionType = EFTTraversalActionType::Hurdle;
	}
	else if (Settings.bAllowMantle && C.bHasFrontLedge && !bThin && bHeightForMantle)
	{
		C.ActionType = EFTTraversalActionType::Mantle;
	}
	else
	{
		C.ActionType = EFTTraversalActionType::None;
	}
}

void UFTTraversalComponent::BuildWarpTargets(const FFTTraversalCandidate& C) const
{
	if (!MotionWarping)
	{
		return;
	}

	static const FName FrontLedgeName = TEXT("FrontLedge");
	static const FName BackLedgeName = TEXT("BackLedge");
	static const FName FloorName = TEXT("Floor");

	// 정렬의 핵심: 캐릭터가 장애물 앞면을 향하도록 X축을 내부 방향(-FrontLedgeNormal)으로 둔다.
	const FRotator FrontFacing = FRotationMatrix::MakeFromX(-C.FrontLedgeNormal).Rotator();
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(FrontLedgeName, C.FrontLedgeLocation, FrontFacing);

	// BackLedge/Floor는 데이터가 있을 때만. 몽타주에 해당 노티파이가 없으면 무시되므로 두어도 무해하다.
	if (C.bHasBackLedge)
	{
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(BackLedgeName, C.BackLedgeLocation, FrontFacing);
	}
	else
	{
		MotionWarping->RemoveWarpTarget(BackLedgeName);
	}

	if (C.bHasFloor)
	{
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(FloorName, C.FloorLocation, FrontFacing);
	}
	else
	{
		MotionWarping->RemoveWarpTarget(FloorName);
	}
}

bool UFTTraversalComponent::TryTraversal()
{
	if (!OwnerCharacter || bIsTraversing)
	{
		return false;
	}

	FFTTraversalCandidate Candidate;
	if (!FindTraversalCandidate(Candidate))
	{
		return false;
	}

	ClassifyTraversal(Candidate);
	if (Candidate.ActionType == EFTTraversalActionType::None)
	{
		return false;
	}

	// 모션매칭/Chooser 대신: 액션별 몽타주 1개 직접 선택(없으면 프로토타입 보간으로 폴백).
	if (const TObjectPtr<UAnimMontage>* Found = MontagesByAction.Find(Candidate.ActionType))
	{
		Candidate.ChosenMontage = *Found;
	}

	StartTraversal(Candidate);
	return true;
}

void UFTTraversalComponent::StartTraversal(const FFTTraversalCandidate& Candidate)
{
	if (!OwnerCharacter)
	{
		return;
	}

	ActiveCandidate = Candidate;
	bIsTraversing = true;

	// 장애물과의 충돌을 액션 동안 무시(끼임 방지).
	if (IsValid(ActiveCandidate.HitComponent))
	{
		OwnerCharacter->GetCapsuleComponent()->IgnoreComponentWhenMoving(ActiveCandidate.HitComponent, true);
	}

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Flying); // 루트모션/보간이 위치를 제어하도록 중력 off
	}

	BuildWarpTargets(ActiveCandidate);
	OnTraversalStarted.Broadcast(ActiveCandidate);

	if (IsValid(ActiveCandidate.ChosenMontage))
	{
		PlayTraversalMontage(ActiveCandidate.ChosenMontage);
	}
	else
	{
		BeginPrototypeInterp(ActiveCandidate);
	}
}

void UFTTraversalComponent::PlayTraversalMontage(UAnimMontage* Montage)
{
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	if (!IsValid(AnimInstance)
		|| AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f) <= 0.0f)
	{
		// 몽타주 재생이 불가하면 보간으로라도 진행(로직 검증 유지).
		BeginPrototypeInterp(ActiveCandidate);
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UFTTraversalComponent::HandleMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UFTTraversalComponent::HandleMontageEnded(UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
{
	FinishTraversal();
}

void UFTTraversalComponent::BeginPrototypeInterp(const FFTTraversalCandidate& Candidate)
{
	InterpStart = OwnerCharacter->GetActorLocation();
	InterpTarget = ComputeInterpTarget(Candidate);
	InterpElapsed = 0.0f;

	switch (Candidate.ActionType)
	{
	case EFTTraversalActionType::Vault:  InterpDuration = Settings.VaultDuration; break;
	case EFTTraversalActionType::Hurdle: InterpDuration = Settings.HurdleDuration; break;
	default:                             InterpDuration = Settings.MantleDuration; break;
	}
	InterpDuration = FMath::Max(0.01f, InterpDuration);

	bInterpolating = true;
	SetComponentTickEnabled(true);
}

FVector UFTTraversalComponent::ComputeInterpTarget(const FFTTraversalCandidate& C) const
{
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	const float R = Capsule->GetScaledCapsuleRadius();
	const float HH = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Into = (-C.FrontLedgeNormal).GetSafeNormal2D();

	switch (C.ActionType)
	{
	case EFTTraversalActionType::Mantle:
		// 앞 모서리 안쪽으로 들어가 윗면 위에 선다.
		return C.FrontLedgeLocation + Into * (R + 10.0f) + FVector(0.0f, 0.0f, HH);

	case EFTTraversalActionType::Hurdle:
	{
		// 뒤 모서리를 지나 반대편 바닥(있으면) 위에 선다.
		const FVector Base = C.bHasFloor ? C.FloorLocation : C.BackLedgeLocation;
		const FVector XY = C.BackLedgeLocation + Into * (R + 10.0f);
		return FVector(XY.X, XY.Y, Base.Z + HH);
	}

	case EFTTraversalActionType::Vault:
	default:
	{
		// 뒤 모서리를 넘긴 지점까지 이동(이후 Falling으로 전환).
		const FVector XY = C.BackLedgeLocation + Into * (R + 10.0f);
		return FVector(XY.X, XY.Y, C.BackLedgeLocation.Z + HH);
	}
	}
}

void UFTTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInterpolating || !OwnerCharacter)
	{
		return;
	}

	InterpElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(InterpElapsed / InterpDuration, 0.0f, 1.0f);
	const float Eased = FMath::SmoothStep(0.0f, 1.0f, Alpha); // 가감속
	OwnerCharacter->SetActorLocation(FMath::Lerp(InterpStart, InterpTarget, Eased), false);

	if (Alpha >= 1.0f)
	{
		FinishTraversal();
	}
}

void UFTTraversalComponent::FinishTraversal()
{
	bInterpolating = false;
	SetComponentTickEnabled(false);
	bIsTraversing = false;

	if (IsValid(ActiveCandidate.HitComponent) && OwnerCharacter)
	{
		OwnerCharacter->GetCapsuleComponent()->IgnoreComponentWhenMoving(ActiveCandidate.HitComponent, false);
	}

	if (OwnerCharacter)
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			// Vault는 반대편으로 낙하, 나머지는 지상 복귀.
			Movement->SetMovementMode(
				ActiveCandidate.ActionType == EFTTraversalActionType::Vault ? MOVE_Falling : MOVE_Walking);
		}
	}
}
