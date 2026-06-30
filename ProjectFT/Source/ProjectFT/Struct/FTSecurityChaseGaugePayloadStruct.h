#pragma once

#include "CoreMinimal.h"
#include "FTSecurityChaseGaugePayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTSecurityChaseGaugePayloadStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	TObjectPtr<AActor> SecurityActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	float ChaseGauge = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	float ChaseGaugeRatio = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|ChaseGauge")
	bool bHasSeenTarget = false;
};
