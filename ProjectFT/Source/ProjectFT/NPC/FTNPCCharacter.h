#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FTNPCCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFTNPCCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
