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
	SetReportContribution(Payload, Payload.ReportAmount);
	ClearReportContribution(Payload.ReporterActor);
}

void UFTReportGaugeComponent::OnReportStarted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	SetReportContribution(Payload, 0.0f);
}

void UFTReportGaugeComponent::OnReportProgress(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	const float NewContribution = Payload.ReportAmount * FMath::Clamp(Payload.ReportProgress, 0.0f, 1.0f);
	SetReportContribution(Payload, NewContribution);
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

void UFTReportGaugeComponent::SetReportContribution(const FFTNPCReportPayloadStruct& Payload, float NewContribution)
{
	if (!Payload.ReporterActor)
	{
		return;
	}

	const TObjectKey<AActor> ReporterKey(Payload.ReporterActor);
	const float PreviousContribution = ActiveReportContributions.FindRef(ReporterKey);
	const float ClampedContribution = FMath::Clamp(NewContribution, 0.0f, Payload.ReportAmount);
	const float DeltaAmount = ClampedContribution - PreviousContribution;

	if (FMath::IsNearlyZero(DeltaAmount))
	{
		return;
	}

	ActiveReportContributions.Add(ReporterKey, ClampedContribution);
	AddReportGauge(DeltaAmount, Payload.ReporterActor, Payload.TargetActor, Payload.ReportLocation);

	if (ClampedContribution <= 0.0f)
	{
		ActiveReportContributions.Remove(ReporterKey);
	}
}

void UFTReportGaugeComponent::ClearReportContribution(AActor* ReportActor)
{
	if (ReportActor)
	{
		ActiveReportContributions.Remove(TObjectKey<AActor>(ReportActor));
	}
}

void UFTReportGaugeComponent::ResetReportGauge()
{
	CurrentReportGauge = 0.0f;
	bSecurityCalled = false;
	ActiveReportContributions.Reset();
}

float UFTReportGaugeComponent::GetReportGaugeRatio() const
{
	if (MaxReportGauge <= 0.0f)
	{
		return 0.0f;
	}

	return CurrentReportGauge / MaxReportGauge;
}
