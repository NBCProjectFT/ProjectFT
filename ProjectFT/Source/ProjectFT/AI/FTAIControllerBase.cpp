#include "FTAIControllerBase.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AFTAIControllerBase::AFTAIControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFTAIControllerBase::ConfigureSight(
	UAIPerceptionComponent* InPerceptionComponent,
	UAISenseConfig_Sight* InSightConfig,
	float SightRadius,
	float PeripheralVisionAngleDegrees,
	float MaxAge)
{
	if (!InPerceptionComponent || !InSightConfig)
	{
		return;
	}

	InSightConfig->SightRadius = SightRadius;
	InSightConfig->LoseSightRadius = SightRadius;
	InSightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	InSightConfig->SetMaxAge(MaxAge);
	InSightConfig->DetectionByAffiliation.bDetectEnemies = true;
	InSightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	InSightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	InPerceptionComponent->ConfigureSense(*InSightConfig);
	InPerceptionComponent->SetDominantSense(InSightConfig->GetSenseImplementation());
	SetPerceptionComponent(*InPerceptionComponent);
}

void AFTAIControllerBase::RefreshSightConfig(
	UAIPerceptionComponent* InPerceptionComponent,
	UAISenseConfig_Sight* InSightConfig) const
{
	if (!InPerceptionComponent || !InSightConfig)
	{
		return;
	}

	InSightConfig->LoseSightRadius = InSightConfig->SightRadius;
	InPerceptionComponent->RequestStimuliListenerUpdate();
}

bool AFTAIControllerBase::IsActorVisibleBySight(AActor* Actor, const UAISenseConfig_Sight* InSightConfig) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !Actor || !InSightConfig)
	{
		return false;
	}

	const FVector ToActor = Actor->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToActor.SizeSquared() > FMath::Square(InSightConfig->SightRadius))
	{
		return false;
	}

	const FVector Forward = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector DirectionToActor = ToActor.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, DirectionToActor);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	if (AngleDegrees > InSightConfig->PeripheralVisionAngleDegrees)
	{
		return false;
	}

	return LineOfSightTo(Actor);
}

void AFTAIControllerBase::DrawFlatSightDebug(
	const UAISenseConfig_Sight* InSightConfig,
	FColor Color,
	float Thickness) const
{
	if (!bDrawSightDebug || !InSightConfig)
	{
		return;
	}

	DrawFlatSectorDebug(
		InSightConfig->SightRadius,
		InSightConfig->PeripheralVisionAngleDegrees,
		Color,
		Thickness);
}

void AFTAIControllerBase::DrawFlatSectorDebug(
	float Radius,
	float HalfAngleDegrees,
	FColor Color,
	float Thickness) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || Radius <= 0.0f)
	{
		return;
	}

	const FVector Origin = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 8.0f);
	const FVector Forward = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	constexpr int32 SegmentCount = 16;
	constexpr float LifeTime = 0.05f;

	FVector PreviousPoint = Origin;
	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float AngleDegrees = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
		const FVector Direction = Forward.RotateAngleAxis(AngleDegrees, FVector::UpVector);
		const FVector CurrentPoint = Origin + Direction * Radius;

		if (SegmentIndex == 0)
		{
			DrawDebugLine(GetWorld(), Origin, CurrentPoint, Color, false, LifeTime, 0, Thickness);
		}
		else
		{
			DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, Color, false, LifeTime, 0, Thickness);
		}

		if (SegmentIndex == SegmentCount)
		{
			DrawDebugLine(GetWorld(), Origin, CurrentPoint, Color, false, LifeTime, 0, Thickness);
		}

		PreviousPoint = CurrentPoint;
	}
}

void AFTAIControllerBase::DrawFlatCircleDebug(
	float Radius,
	FColor Color,
	float Thickness) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!bDrawSightDebug || !ControlledPawn || Radius <= 0.0f)
	{
		return;
	}

	const FVector Origin = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 10.0f);
	constexpr int32 SegmentCount = 32;
	constexpr float LifeTime = 0.05f;

	DrawDebugCircle(
		GetWorld(),
		Origin,
		Radius,
		SegmentCount,
		Color,
		false,
		LifeTime,
		0,
		Thickness,
		FVector::ForwardVector,
		FVector::RightVector,
		false
	);
}
