#include "FTNPCReportGaugeWidget.h"

#include "Components/ProgressBar.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

void UFTNPCReportGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateReportProgress(0.0f);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ReportGaugeChangedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ReportGaugeChanged,
		this,
		&ThisClass::OnReportGaugeChanged
	);
}

void UFTNPCReportGaugeWidget::NativeDestruct()
{
	if (ReportGaugeChangedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ReportGaugeChangedListenerHandle);
		ReportGaugeChangedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::NativeDestruct();
}

void UFTNPCReportGaugeWidget::SetReportOwnerActor(AActor* InReportOwnerActor)
{
	ReportOwnerActor = InReportOwnerActor;
}

void UFTNPCReportGaugeWidget::OnReportGaugeChanged(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (!ReportOwnerActor || Payload.ReporterActor != ReportOwnerActor)
	{
		return;
	}

	UpdateReportProgress(Payload.ReportProgress);
}

void UFTNPCReportGaugeWidget::UpdateReportProgress(float ReportProgress)
{
	const float ClampedReportProgress = FMath::Clamp(ReportProgress, 0.0f, 1.0f);

	if (ReportProgressBar)
	{
		ReportProgressBar->SetPercent(ClampedReportProgress);
		ReportProgressBar->SetVisibility(!bHideWhenEmpty || ClampedReportProgress > 0.0f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
