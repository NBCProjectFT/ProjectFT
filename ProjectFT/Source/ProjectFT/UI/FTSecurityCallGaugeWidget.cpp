#include "FTSecurityCallGaugeWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

void UFTSecurityCallGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateSecurityCallProgress(0.0f);
	SetSecurityCallCompleted(false);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	SecurityCallGaugeChangedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCallGaugeChanged,
		this,
		&ThisClass::OnSecurityCallGaugeChanged
	);
}

void UFTSecurityCallGaugeWidget::NativeDestruct()
{
	if (SecurityCallGaugeChangedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityCallGaugeChangedListenerHandle);
		SecurityCallGaugeChangedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::NativeDestruct();
}

void UFTSecurityCallGaugeWidget::SetSecurityOwnerActor(AActor* InSecurityOwnerActor)
{
	SecurityOwnerActor = InSecurityOwnerActor;
}

void UFTSecurityCallGaugeWidget::OnSecurityCallGaugeChanged(
	FGameplayTag Channel,
	const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (!SecurityOwnerActor || Payload.SecurityActor != SecurityOwnerActor)
	{
		return;
	}

	UpdateSecurityCallProgress(Payload.ChaseGaugeRatio);
}

void UFTSecurityCallGaugeWidget::UpdateSecurityCallProgress(float Progress)
{
	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	const bool bCompleted = ClampedProgress >= 1.0f;

	SetSecurityCallCompleted(bCompleted);

	if (SecurityCallProgressBar)
	{
		SecurityCallProgressBar->SetPercent(ClampedProgress);
		SecurityCallProgressBar->SetVisibility(!bCompleted && (!bHideWhenEmpty || ClampedProgress > 0.0f)
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UFTSecurityCallGaugeWidget::SetSecurityCallCompleted(bool bCompleted)
{
	if (SecurityCallIconImage)
	{
		SecurityCallIconImage->SetVisibility(bCompleted
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
