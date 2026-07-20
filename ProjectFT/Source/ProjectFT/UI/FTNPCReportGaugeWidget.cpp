#include "FTNPCReportGaugeWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "TimerManager.h"

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
	// 위젯이 제거될 때 아직 예약된 아이콘 숨긴 타이머가 남아있으면 정리합니다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReportIconTimerHandle);
	}

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
	if (!ReportIconImage)
	{
		return;
	}

	// 기존에 돌고 있던 아이콘 숨김 타이머를 먼저 취소합니다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReportIconTimerHandle);
	}

	// bCompleted true면 아이콘을 보이고, false면 숨깁니다.
	ReportIconImage->SetVisibility(bCompleted
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);

	// 신고가 완료되었을때
	if (bCompleted)
	{
		if (UWorld* World = GetWorld())
		{
			// ReportIconDisplayDuration초 뒤에 HideReportIcon()을 한 번 호출합니다.
			World->GetTimerManager().SetTimer(
				ReportIconTimerHandle,
				this,
				&ThisClass::HideReportIcon,
				ReportIconDisplayDuration,
				false
			);
		}
	}
}

void UFTNPCReportGaugeWidget::HideReportIcon()
{
	if (ReportIconImage)
	{
		// ReportIconImage만 숨깁니다.
		ReportIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
