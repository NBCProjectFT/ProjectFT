#pragma once

#include "CoreMinimal.h"
#include "FTNPCReportPayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTNPCReportPayloadStruct
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|NPC|Report")
	TObjectPtr<AActor> ReporterActor = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|NPC|Report")
	TObjectPtr<AActor> TargetActor = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|NPC|Report")
	FVector ReportLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|NPC|Report")
	float ReportAmount = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|NPC|Report")
	float ReportProgress = 0.0f;
};
