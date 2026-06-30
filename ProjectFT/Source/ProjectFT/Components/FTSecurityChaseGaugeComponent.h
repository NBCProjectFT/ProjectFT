#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "FTSecurityChaseGaugeComponent.generated.h"

struct FFTNPCReportPayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityChaseGaugeComponent : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	UFTSecurityChaseGaugeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	float GetChaseGauge() const;

	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	float GetChaseGaugeRatio() const;

	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	bool IsChaseActive() const;

	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	bool IsAnySecuritySeeingTarget() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Security|ChaseGauge")
	void ResetChaseGauge();

protected:
	virtual void StartListening() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge", meta = (ClampMin = "1.0"))
	float MaxChaseGauge = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge", meta = (ClampMin = "0.0"))
	float ChaseGaugeDecayPerSecond = 10.0f;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "FT|Security|ChaseGauge")
	float CurrentChaseGauge = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "FT|Security|ChaseGauge")
	bool bChaseActive = false;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	FVector LastKnownLocation = FVector::ZeroVector;
	TSet<TWeakObjectPtr<AActor>> SecuritiesSeeingTarget;

	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnSecurityTargetSeen(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnSecurityTargetLost(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void SetChaseGauge(float NewChaseGauge);
	void BroadcastGaugeChanged();
	void BroadcastChaseEnded();
	void RemoveInvalidSecurityActors();
};
