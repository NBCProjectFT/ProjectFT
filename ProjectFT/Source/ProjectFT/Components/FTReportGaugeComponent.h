#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "FTReportGaugeComponent.generated.h"

struct FFTNPCReportPayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTReportGaugeComponent : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	UFTReportGaugeComponent();

	void AddReportGauge(float Amount, AActor* ReportActor, AActor* TargetActor, FVector ReportLocation);
	void ResetReportGauge();
	float GetReportGaugeRatio() const;

protected:
	virtual void StartListening() override;
	virtual void StopListening() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|ReportGauge")
	float MaxReportGauge = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|ReportGauge")
	float CurrentReportGauge = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|ReportGauge")
	bool bSecurityCalled = false;

private:
	void OnReportCompleted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
};
