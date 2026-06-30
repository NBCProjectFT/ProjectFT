#include "FTSecurityChaseGaugeWidget.h"

#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "ProjectFT/Components/FTSecurityChaseGaugeComponent.h"
#include "ProjectFT/Core/FTGameState.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

void UFTSecurityChaseGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateChaseGauge(0.0f, false);

	if (const AFTGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr)
	{
		if (const UFTSecurityChaseGaugeComponent* ChaseGaugeComponent = GameState->SecurityChaseGaugeComponent)
		{
			UpdateChaseGauge(ChaseGaugeComponent->GetChaseGaugeRatio(), ChaseGaugeComponent->IsChaseActive());
		}
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ChaseGaugeChangedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityChaseGaugeChanged,
		this,
		&ThisClass::OnChaseGaugeChanged
	);
	ChaseEndedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityChaseEnded,
		this,
		&ThisClass::OnChaseEnded
	);
}

void UFTSecurityChaseGaugeWidget::NativeDestruct()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	if (ChaseGaugeChangedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(ChaseGaugeChangedListenerHandle);
		ChaseGaugeChangedListenerHandle = FGameplayMessageListenerHandle();
	}

	if (ChaseEndedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(ChaseEndedListenerHandle);
		ChaseEndedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::NativeDestruct();
}

void UFTSecurityChaseGaugeWidget::OnChaseGaugeChanged(
	FGameplayTag Channel,
	const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	UpdateChaseGauge(Payload.ChaseGaugeRatio, Payload.ChaseGauge > 0.0f);
}

void UFTSecurityChaseGaugeWidget::OnChaseEnded(
	FGameplayTag Channel,
	const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	UpdateChaseGauge(0.0f, false);
}

void UFTSecurityChaseGaugeWidget::UpdateChaseGauge(float ChaseGaugeRatio, bool bChaseActive)
{
	if (ChaseGaugeProgressBar)
	{
		ChaseGaugeProgressBar->SetPercent(FMath::Clamp(ChaseGaugeRatio, 0.0f, 1.0f));
	}

	SetVisibility(bChaseActive
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}
