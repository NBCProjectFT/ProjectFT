#include "FTSecurityChaseGaugeComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

UFTSecurityChaseGaugeComponent::UFTSecurityChaseGaugeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFTSecurityChaseGaugeComponent::StartListening()
{
	Super::StartListening();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCalled,
		this,
		&ThisClass::OnSecurityCalled
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetSeen,
		this,
		&ThisClass::OnSecurityTargetSeen
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetLost,
		this,
		&ThisClass::OnSecurityTargetLost
	));
}

void UFTSecurityChaseGaugeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bChaseActive)
	{
		return;
	}

	RemoveInvalidSecurityActors();
	if (!SecuritiesSeeingTarget.IsEmpty())
	{
		return;
	}

	SetChaseGauge(CurrentChaseGauge - ChaseGaugeDecayPerSecond * DeltaTime);
}

void UFTSecurityChaseGaugeComponent::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	TargetActor = Payload.TargetActor;
	LastKnownLocation = Payload.ReportLocation;
	bChaseActive = true;
	SetChaseGauge(MaxChaseGauge);
}

void UFTSecurityChaseGaugeComponent::OnSecurityTargetSeen(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (!Payload.SecurityActor)
	{
		return;
	}

	SecuritiesSeeingTarget.Add(TWeakObjectPtr<AActor>(Payload.SecurityActor.Get()));
	TargetActor = Payload.TargetActor;
	LastKnownLocation = Payload.LastKnownLocation;
	bChaseActive = true;

	if (!FMath::IsNearlyEqual(CurrentChaseGauge, MaxChaseGauge))
	{
		SetChaseGauge(MaxChaseGauge);
		return;
	}

	BroadcastGaugeChanged();
}

void UFTSecurityChaseGaugeComponent::OnSecurityTargetLost(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (Payload.SecurityActor)
	{
		SecuritiesSeeingTarget.Remove(TWeakObjectPtr<AActor>(Payload.SecurityActor.Get()));
	}

	if (!Payload.LastKnownLocation.IsNearlyZero())
	{
		LastKnownLocation = Payload.LastKnownLocation;
	}

	if (bChaseActive)
	{
		BroadcastGaugeChanged();
	}
}

void UFTSecurityChaseGaugeComponent::SetChaseGauge(float NewChaseGauge)
{
	const float ClampedChaseGauge = FMath::Clamp(NewChaseGauge, 0.0f, MaxChaseGauge);
	if (FMath::IsNearlyEqual(CurrentChaseGauge, ClampedChaseGauge))
	{
		return;
	}

	CurrentChaseGauge = ClampedChaseGauge;
	BroadcastGaugeChanged();

	if (CurrentChaseGauge <= 0.0f && bChaseActive)
	{
		bChaseActive = false;
		SecuritiesSeeingTarget.Reset();
		BroadcastChaseEnded();
	}
}

void UFTSecurityChaseGaugeComponent::BroadcastGaugeChanged()
{
	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = LastKnownLocation;
	Payload.ChaseGauge = CurrentChaseGauge;
	Payload.ChaseGaugeRatio = GetChaseGaugeRatio();
	Payload.bHasSeenTarget = !SecuritiesSeeingTarget.IsEmpty();

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityChaseGaugeChanged, Payload);
}

void UFTSecurityChaseGaugeComponent::BroadcastChaseEnded()
{
	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = LastKnownLocation;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityChaseEnded, Payload);
}

void UFTSecurityChaseGaugeComponent::RemoveInvalidSecurityActors()
{
	for (auto SecurityIterator = SecuritiesSeeingTarget.CreateIterator(); SecurityIterator; ++SecurityIterator)
	{
		if (!SecurityIterator->IsValid())
		{
			SecurityIterator.RemoveCurrent();
		}
	}
}

void UFTSecurityChaseGaugeComponent::ResetChaseGauge()
{
	const bool bWasChaseActive = bChaseActive;
	CurrentChaseGauge = 0.0f;
	bChaseActive = false;
	SecuritiesSeeingTarget.Reset();
	BroadcastGaugeChanged();
	if (bWasChaseActive)
	{
		BroadcastChaseEnded();
	}

	TargetActor = nullptr;
	LastKnownLocation = FVector::ZeroVector;
}

float UFTSecurityChaseGaugeComponent::GetChaseGauge() const
{
	return CurrentChaseGauge;
}

float UFTSecurityChaseGaugeComponent::GetChaseGaugeRatio() const
{
	return MaxChaseGauge > 0.0f ? CurrentChaseGauge / MaxChaseGauge : 0.0f;
}

bool UFTSecurityChaseGaugeComponent::IsChaseActive() const
{
	return bChaseActive;
}

bool UFTSecurityChaseGaugeComponent::IsAnySecuritySeeingTarget() const
{
	return !SecuritiesSeeingTarget.IsEmpty();
}
