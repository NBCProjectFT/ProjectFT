#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "FTCharacterAnimInstance.generated.h"

class ACharacter;
class AFTCharacterBase;
class UCharacterMovementComponent;

UCLASS()
class PROJECTFT_API UFTCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual void CacheOwnerReferences();
	virtual void UpdateCharacterState(float DeltaSeconds);

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<AFTCharacterBase> FTCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Reference")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	float ForwardSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	float StrafeSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	bool bCanStrafe = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|Movement")
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|State")
	bool bIsImmobilized = false;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Anim|State")
	FGameplayTag ImmobilizedTag;
};
