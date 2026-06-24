#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "FTNPCCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTNPCCharacter : public AFTAICharacterBase
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
