#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Animation/FTAIAnimInstance.h"
#include "FTSecurityAnimInstance.generated.h"

class AFTSecurityAIController;
class AFTSecurityCharacter;

UCLASS()
class PROJECTFT_API UFTSecurityAnimInstance : public UFTAIAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void CacheOwnerReferences() override;
	virtual void UpdateCharacterState(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTSecurityCharacter> SecurityCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTSecurityAIController> SecurityAIController;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsGrabbing = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsCaptor = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bTargetCaptured = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsTargetCapturedByOtherSecurity = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsReturning = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsAttackLeader = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bIsTargetInAttackRange = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Security")
	bool bSecurityChaseActive = false;
};
