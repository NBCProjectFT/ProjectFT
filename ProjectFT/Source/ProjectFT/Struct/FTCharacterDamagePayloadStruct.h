#pragma once

#include "CoreMinimal.h"
#include "FTCharacterDamagePayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTCharacterDamagePayloadStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Damage")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Damage")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Damage")
	float DamageAmount = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Damage")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Damage")
	bool bTargetKnockedOut = false;
};
