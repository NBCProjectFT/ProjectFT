#pragma once

#include "CoreMinimal.h"
#include "FTDamageTextPayloadStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTDamageTextPayloadStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|DamageText")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|DamageText")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|DamageText")
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|DamageText")
	FVector HitLocation = FVector::ZeroVector;
};
