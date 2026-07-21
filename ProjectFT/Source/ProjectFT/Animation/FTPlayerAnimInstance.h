#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Animation/FTCharacterAnimInstance.h"
#include "ProjectFT/Enum/FTWeaponStanceType.h"
#include "FTPlayerAnimInstance.generated.h"

class AFTPlayerCharacter;

UCLASS()
class PROJECTFT_API UFTPlayerAnimInstance : public UFTCharacterAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void CacheOwnerReferences() override;
	virtual void UpdateCharacterState(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTPlayerCharacter> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	bool bIsCaptured = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	bool bIsChannelingInteraction = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	bool bIsInventoryOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	EFTWeaponStanceType HeldWeaponStance = EFTWeaponStanceType::Unarmed;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	float SprintMovementSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	float CrouchMovementSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Anim|Player")
	float RunningReferenceSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Anim|Player")
	float CrouchReferenceSpeed = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	float NormalizedGroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	float LocomotionPlayRate = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Player")
	float CrouchPlayRate = 1.0f;
};
