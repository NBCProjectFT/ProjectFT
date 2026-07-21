#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Animation/FTAIAnimInstance.h"
#include "FTNPCAnimInstance.generated.h"

class AFTNPCAIController;
class AFTNPCCharacter;

UCLASS()
class PROJECTFT_API UFTNPCAnimInstance : public UFTAIAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void CacheOwnerReferences() override;
	virtual void UpdateCharacterState(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTNPCCharacter> NPCCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTNPCAIController> NPCAIController;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bIsShopping = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bIsReporting = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bIsPanicking = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bIsFleeing = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bIsKnockedOut = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	bool bObservedThreat = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|NPC")
	float ReportProgress = 0.0f;
};
