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
	
	// NPC 신고 관련 메시지를 수신해 Reporter별 게이지 상태를 갱신한다.
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

void UFTReportGaugeComponent::OnReportCompleted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	SetReporterGauge(Payload, MaxReportGauge);
}

void UFTReportGaugeComponent::OnReportStarted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (Payload.ReporterActor)
	{
		// 같은 Reporter가 다시 신고를 시작할 수 있도록 보안요원 호출 기록을 초기화한다.
		SecurityCalledReporters.Remove(TObjectKey<AActor>(Payload.ReporterActor));
	}
	
	// 신고 시작 시 해당 Reporter의 게이지를 0으로 초기화한다.
	SetReporterGauge(Payload, 0.0f);
}

void UFTReportGaugeComponent::OnReportProgress(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	// ReportProgress의 0~1 비율을 실제 게이지 값으로 변환한다.
	const float NewReportGauge = MaxReportGauge * FMath::Clamp(Payload.ReportProgress, 0.0f, 1.0f);
	SetReporterGauge(Payload, NewReportGauge);
}

void UFTReportGaugeComponent::AddReportGauge(const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.ReporterActor)
	{
		return;
	}
	
	// 기존 Reporter 게이지에 Payload의 ReportAmount를 누적한다.
	const TObjectKey<AActor> ReporterKey(Payload.ReporterActor);
	SetReporterGauge(Payload, ReportGaugeByReporter.FindRef(ReporterKey) + Payload.ReportAmount);
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
	
	// 게이지 값이 실질적으로 변하지 않았다면 불필요한 메시지 발행을 막는다.
	if (FMath::IsNearlyEqual(PreviousReportGauge, ClampedReportGauge))
	{
		return;
	}

	ReportGaugeByReporter.Add(ReporterKey, ClampedReportGauge);
	BroadcastReporterGaugeChanged(Payload);

	// 게이지가 0 이하가 되면 Map에서 제거해 불필요한 Reporter 데이터를 남기지 않는다.
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
	
	// UI나 다른 시스템에서 사용할 수 있도록 현재 게이지 정보를 Payload로 재구성한다.
	FFTNPCReportPayloadStruct GaugePayload;
	GaugePayload.ReporterActor = Payload.ReporterActor;
	GaugePayload.TargetActor = Payload.TargetActor;
	GaugePayload.ReportLocation = Payload.ReportLocation;
	GaugePayload.ReportAmount = CurrentReportGauge;
	GaugePayload.ReportProgress = GetReportGaugeRatio(Payload.ReporterActor);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	
	// Reporter별 신고 게이지 변경 알림
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ReportGaugeChanged, GaugePayload);

	const TObjectKey<AActor> ReporterKey(Payload.ReporterActor);
	
	// 같은 Reporter가 중복으로 보안 호출을 발생시키지 않도록 한 번만 방송한다.
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
		// Reporter의 게이지와 보안 호출 기록을 함께 제거한다.
		const TObjectKey<AActor> ReporterKey(ReportActor);
		ReportGaugeByReporter.Remove(ReporterKey);
		SecurityCalledReporters.Remove(ReporterKey);
	}
}

void UFTReportGaugeComponent::ResetAllReportGauges()
{
	// 모든 Reporter의 신고 상태를 초기화한다.
	ReportGaugeByReporter.Reset();
	SecurityCalledReporters.Reset();
}

float UFTReportGaugeComponent::GetHighestReportGaugeRatio() const
{
	if (MaxReportGauge <= 0.0f)
	{
		return 0.0f;
	}

	// 현재 신고 중인 Reporter들 중 가장 높은 값을 찾는다.
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

	// 특정 Reporter의 현재 신고 게이지 비율을 반환한다.
	return ReportGaugeByReporter.FindRef(TObjectKey<AActor>(ReportActor)) / MaxReportGauge;
}
