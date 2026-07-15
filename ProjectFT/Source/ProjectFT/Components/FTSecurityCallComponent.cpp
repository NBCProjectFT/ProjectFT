#include "FTSecurityCallComponent.h"

#include "AIController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

UFTSecurityCallComponent::UFTSecurityCallComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityCallComponent::StartSecurityCall(AActor* InTargetActor)
{
	if (!InTargetActor)
	{
		return;
	}

	TargetActor = InTargetActor;
	SecurityCallProgress = 0.0f;
	bSecurityCallCompleted = false;
	BroadcastGaugeChanged(InTargetActor->GetActorLocation(), true);
}

void UFTSecurityCallComponent::StopSecurityCall()
{
	TargetActor = nullptr;
	SecurityCallProgress = 0.0f;
	bSecurityCallCompleted = false;
	BroadcastGaugeChanged(FVector::ZeroVector, false);
}

void UFTSecurityCallComponent::TickSecurityCall(
	float DeltaTime,
	bool bCanCharge,
	const FVector& LastKnownLocation,
	bool bHasSeenTarget)
{
	if (!TargetActor || bSecurityCallCompleted || !bCanCharge)
	{
		return;
	}

	const float PreviousProgress = SecurityCallProgress;
	SecurityCallProgress = FMath::Clamp(
		SecurityCallProgress + DeltaTime / FMath::Max(SecurityCallDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	if (!FMath::IsNearlyEqual(PreviousProgress, SecurityCallProgress))
	{
		BroadcastGaugeChanged(LastKnownLocation, bHasSeenTarget);
	}

	if (SecurityCallProgress >= 1.0f)
	{
		CompleteSecurityCall(LastKnownLocation, bHasSeenTarget);
	}
}

void UFTSecurityCallComponent::BroadcastGaugeChanged(const FVector& LastKnownLocation, bool bHasSeenTarget) const
{
	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.SecurityActor = GetSecurityActor();
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = LastKnownLocation;
	Payload.ChaseGaugeRatio = SecurityCallProgress;
	Payload.bHasSeenTarget = bHasSeenTarget;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityCallGaugeChanged, Payload);
}

void UFTSecurityCallComponent::CompleteSecurityCall(const FVector& LastKnownLocation, bool bHasSeenTarget)
{
	if (bSecurityCallCompleted)
	{
		return;
	}

	bSecurityCallCompleted = true;
	SecurityCallProgress = 1.0f;
	BroadcastGaugeChanged(LastKnownLocation, bHasSeenTarget);

	if (!TargetActor)
	{
		return;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = GetSecurityActor();
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = LastKnownLocation.IsNearlyZero() ? TargetActor->GetActorLocation() : LastKnownLocation;
	Payload.ReportAmount = 0.0f;
	Payload.ReportProgress = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityCalled, Payload);
}

AActor* UFTSecurityCallComponent::GetSecurityActor() const
{
	const AAIController* OwnerController = Cast<AAIController>(GetOwner());
	return OwnerController ? OwnerController->GetPawn() : GetOwner();
}
