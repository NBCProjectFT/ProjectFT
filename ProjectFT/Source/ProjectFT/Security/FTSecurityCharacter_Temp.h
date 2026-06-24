#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FTSecurityCharacter_Temp.generated.h"

UCLASS()
class PROJECTFT_API AFTSecurityCharacter_Temp : public ACharacter
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter_Temp();

	UFUNCTION(BlueprintCallable)
	void SetMoveSpeed(float NewSpeed);

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
