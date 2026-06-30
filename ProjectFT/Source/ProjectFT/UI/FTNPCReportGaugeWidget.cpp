#include "FTNPCReportGaugeWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

void UFTNPCReportGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateReportProgress(0.0f);
	SetReportCompleted(false);

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
	const bool bReportCompleted = ClampedReportProgress >= 1.0f;

	SetReportCompleted(bReportCompleted);

	if (ReportProgressBar)
	{
		ReportProgressBar->SetPercent(ClampedReportProgress);
		ReportProgressBar->SetVisibility(!bReportCompleted && (!bHideWhenEmpty || ClampedReportProgress > 0.0f)
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UFTNPCReportGaugeWidget::SetReportCompleted(bool bCompleted)
{
	if (ReportIconImage)
	{
		ReportIconImage->SetVisibility(bCompleted
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
