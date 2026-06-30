#pragma once

#include "CoreMinimal.h"
#include "FTSecurityResponsePayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTSecurityResponsePayloadStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|Response")
	TObjectPtr<AActor> SecurityActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|Response")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|Response")
	TObjectPtr<AActor> SecurityRoomActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|Response")
	FVector ReportLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Security|Response")
	FVector ReturnLocation = FVector::ZeroVector;
};
