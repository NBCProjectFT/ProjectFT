#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "TimerManager.h"
#include "FTSecurityCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTSecurityCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter();
	
	virtual void SetMoveSpeed(float NewSpeed) override;
	void IgnorePawnCollisionForDuration(float Duration);
	void SetPawnCollisionIgnored(bool bIgnored);
	void RestorePawnCollision();

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FTimerHandle PawnCollisionRestoreTimerHandle;
	ECollisionResponse DefaultPawnCollisionResponse = ECR_Block;
};
