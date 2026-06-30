#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTSecurityChaseGaugeWidget.generated.h"

class UProgressBar;
struct FFTSecurityChaseGaugePayloadStruct;

UCLASS()
class PROJECTFT_API UFTSecurityChaseGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ChaseGaugeProgressBar;

private:
	FGameplayMessageListenerHandle ChaseGaugeChangedListenerHandle;
	FGameplayMessageListenerHandle ChaseEndedListenerHandle;

	void OnChaseGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void UpdateChaseGauge(float ChaseGaugeRatio, bool bChaseActive);
};
