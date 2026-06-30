#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTCharacterBase.h"
#include "FTAICharacterBase.generated.h"

UCLASS(Abstract)
class PROJECTFT_API AFTAICharacterBase : public AFTCharacterBase
{
	GENERATED_BODY()

public:
	AFTAICharacterBase();

	UFUNCTION(BlueprintCallable, Category = "FT|AI|Movement")
	virtual void SetMoveSpeed(float NewSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Stat", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Movement", meta = (ClampMin = "0.0"))
	float InitialMoveSpeed = 300.0f;
};
