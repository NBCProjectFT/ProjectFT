#pragma once

#include "CoreMinimal.h"
#include "FTThrowActorStruct.generated.h"

class UAnimMontage;
class UFTProjectileActorDataAsset;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTThrowActorStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Animation")
	TObjectPtr<UAnimMontage> PrepareMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Projectile")
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Input")
	bool bUseTwoStepThrow = true;
};
