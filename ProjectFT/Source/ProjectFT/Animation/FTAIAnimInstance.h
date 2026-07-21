#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Animation/FTCharacterAnimInstance.h"
#include "FTAIAnimInstance.generated.h"

class AFTAICharacterBase;

UCLASS()
class PROJECTFT_API UFTAIAnimInstance : public UFTCharacterAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void CacheOwnerReferences() override;
	virtual void UpdateCharacterState(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTAICharacterBase> AICharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Anim|AI")
	float MovementReferenceSpeed = 600.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|AI")
	float NormalizedGroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|AI")
	float StrafeDirection = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|AI")
	bool bIsStunned = false;
};
