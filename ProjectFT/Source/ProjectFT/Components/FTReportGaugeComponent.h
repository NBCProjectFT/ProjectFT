#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "UObject/ObjectKey.h"
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
	float GetReportGaugeRatio(AActor* ReportActor) const;

protected:
	virtual void StartListening() override;
	virtual void StopListening() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|ReportGauge")
	float MaxReportGauge = 100.0f;

private:
	TMap<TObjectKey<AActor>, float> ReportGaugeByReporter;
	TSet<TObjectKey<AActor>> SecurityCalledReporters;

	void OnReportStarted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnReportProgress(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnReportCompleted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void SetReporterGauge(const FFTNPCReportPayloadStruct& Payload, float NewReportGauge);
	void ClearReporterGauge(AActor* ReportActor);
	void BroadcastReporterGaugeChanged(const FFTNPCReportPayloadStruct& Payload);
};
