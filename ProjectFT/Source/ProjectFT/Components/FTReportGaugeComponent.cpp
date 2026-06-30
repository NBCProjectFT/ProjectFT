#include "FTReportGaugeComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTReportGaugeComponent::UFTReportGaugeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTReportGaugeComponent::StartListening()
{
	Super::StartListening();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_NPCReportStarted,
		this,
		&ThisClass::OnReportStarted
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_NPCReportProgress,
		this,
		&ThisClass::OnReportProgress
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_NPCReportCompleted,
		this,
		&ThisClass::OnReportCompleted
	));
}

void UFTReportGaugeComponent::StopListening()
{
	Super::StopListening();
}

void UFTReportGaugeComponent::OnReportCompleted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	SetReporterGauge(Payload, MaxReportGauge);
}

void UFTReportGaugeComponent::OnReportStarted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (Payload.ReporterActor)
	{
		SecurityCalledReporters.Remove(TObjectKey<AActor>(Payload.ReporterActor));
	}

	SetReporterGauge(Payload, 0.0f);
}

void UFTReportGaugeComponent::OnReportProgress(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	const float NewReportGauge = MaxReportGauge * FMath::Clamp(Payload.ReportProgress, 0.0f, 1.0f);
	SetReporterGauge(Payload, NewReportGauge);
}

void UFTReportGaugeComponent::AddReportGauge(float Amount, AActor* ReportActor, AActor* TargetActor, FVector ReportLocation)
{
	if (!ReportActor)
	{
		return;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = ReportActor;
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = ReportLocation;

	const TObjectKey<AActor> ReporterKey(ReportActor);
	SetReporterGauge(Payload, ReportGaugeByReporter.FindRef(ReporterKey) + Amount);
}

void UFTReportGaugeComponent::SetReporterGauge(const FFTNPCReportPayloadStruct& Payload, float NewReportGauge)
{
	if (!Payload.ReporterActor)
	{
		return;
	}

	const TObjectKey<AActor> ReporterKey(Payload.ReporterActor);
	const float PreviousReportGauge = ReportGaugeByReporter.FindRef(ReporterKey);
	const float ClampedReportGauge = FMath::Clamp(NewReportGauge, 0.0f, MaxReportGauge);

	if (FMath::IsNearlyEqual(PreviousReportGauge, ClampedReportGauge))
	{
		return;
	}

	ReportGaugeByReporter.Add(ReporterKey, ClampedReportGauge);
	BroadcastReporterGaugeChanged(Payload);

	if (ClampedReportGauge <= 0.0f)
	{
		ClearReporterGauge(Payload.ReporterActor);
	}
}

void UFTReportGaugeComponent::BroadcastReporterGaugeChanged(const FFTNPCReportPayloadStruct& Payload)
{
	const float CurrentReportGauge = Payload.ReporterActor
		? ReportGaugeByReporter.FindRef(TObjectKey<AActor>(Payload.ReporterActor))
		: 0.0f;
	
	FFTNPCReportPayloadStruct GaugePayload;
	GaugePayload.ReporterActor = Payload.ReporterActor;
	GaugePayload.TargetActor = Payload.TargetActor;
	GaugePayload.ReportLocation = Payload.ReportLocation;
	GaugePayload.ReportAmount = CurrentReportGauge;
	GaugePayload.ReportProgress = GetReportGaugeRatio(Payload.ReporterActor);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ReportGaugeChanged, GaugePayload);

	const TObjectKey<AActor> ReporterKey(Payload.ReporterActor);
	if (Payload.ReporterActor && !SecurityCalledReporters.Contains(ReporterKey) && CurrentReportGauge >= MaxReportGauge)
	{
		SecurityCalledReporters.Add(ReporterKey);
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_SecurityCalled, GaugePayload);
	}
}

void UFTReportGaugeComponent::ClearReporterGauge(AActor* ReportActor)
{
	if (ReportActor)
	{
		const TObjectKey<AActor> ReporterKey(ReportActor);
		ReportGaugeByReporter.Remove(ReporterKey);
		SecurityCalledReporters.Remove(ReporterKey);
	}
}

void UFTReportGaugeComponent::ResetReportGauge()
{
	ReportGaugeByReporter.Reset();
	SecurityCalledReporters.Reset();
}

float UFTReportGaugeComponent::GetReportGaugeRatio() const
{
	if (MaxReportGauge <= 0.0f)
	{
		return 0.0f;
	}

	float HighestReportGauge = 0.0f;
	for (const TPair<TObjectKey<AActor>, float>& ReportGaugePair : ReportGaugeByReporter)
	{
		HighestReportGauge = FMath::Max(HighestReportGauge, ReportGaugePair.Value);
	}

	return HighestReportGauge / MaxReportGauge;
}

float UFTReportGaugeComponent::GetReportGaugeRatio(AActor* ReportActor) const
{
	if (!ReportActor || MaxReportGauge <= 0.0f)
	{
		return 0.0f;
	}

	return ReportGaugeByReporter.FindRef(TObjectKey<AActor>(ReportActor)) / MaxReportGauge;
}
