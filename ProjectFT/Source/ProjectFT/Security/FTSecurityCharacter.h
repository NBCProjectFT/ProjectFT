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

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
