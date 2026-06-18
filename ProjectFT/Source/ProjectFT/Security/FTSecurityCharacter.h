#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FTSecurityCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTSecurityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter();
	
	UFUNCTION(BlueprintCallable)
	void SetMoveSpeed(float NewSpeed);

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
