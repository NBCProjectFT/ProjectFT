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
	AddReportGauge(Payload.ReportAmount, Payload.ReporterActor, Payload.TargetActor, Payload.ReportLocation);
}

void UFTReportGaugeComponent::AddReportGauge(float Amount, AActor* ReportActor, AActor* TargetActor, FVector ReportLocation)
{
	CurrentReportGauge = FMath::Clamp(CurrentReportGauge + Amount, 0.0f, MaxReportGauge);

	FFTNPCReportPayloadStruct GaugePayload;
	GaugePayload.ReporterActor = ReportActor;
	GaugePayload.TargetActor = TargetActor;
	GaugePayload.ReportLocation = ReportLocation;
	GaugePayload.ReportAmount = Amount;
	GaugePayload.ReportProgress = GetReportGaugeRatio();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ReportGaugeChanged, GaugePayload);

	if (!bSecurityCalled && CurrentReportGauge >= MaxReportGauge)
	{
		bSecurityCalled = true;
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_SecurityCalled, GaugePayload);
	}
}

void UFTReportGaugeComponent::ResetReportGauge()
{
	CurrentReportGauge = 0.0f;
	bSecurityCalled = false;
}

float UFTReportGaugeComponent::GetReportGaugeRatio() const
{
	if (MaxReportGauge <= 0.0f)
	{
		return 0.0f;
	}

	return CurrentReportGauge / MaxReportGauge;
}
