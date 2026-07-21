#include "FTCharacterAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/Character/FTCharacterBase.h"

void UFTCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwnerReferences();
}

void UFTCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		CacheOwnerReferences();
	}

	UpdateCharacterState(DeltaSeconds);
}

void UFTCharacterAnimInstance::CacheOwnerReferences()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	FTCharacter = Cast<AFTCharacterBase>(OwnerCharacter);
	MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
}

void UFTCharacterAnimInstance::UpdateCharacterState(float DeltaSeconds)
{
	if (!OwnerCharacter)
	{
		Velocity = FVector::ZeroVector;
		GroundSpeed = 0.0f;
		ForwardSpeed = 0.0f;
		StrafeSpeed = 0.0f;
		bCanStrafe = false;
		bShouldMove = false;
		bIsFalling = false;
		bIsCrouching = false;
		bIsDead = false;
		bIsImmobilized = false;
		ImmobilizedTag = FGameplayTag();
		return;
	}

	Velocity = OwnerCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bIsFalling = MovementComponent ? MovementComponent->IsFalling() : false;
	bIsCrouching = OwnerCharacter->bIsCrouched;
	bCanStrafe = MovementComponent ? !MovementComponent->bOrientRotationToMovement : false;

	FVector PlanarVelocity = Velocity;
	PlanarVelocity.Z = 0.0f;

	FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
	ForwardVector.Z = 0.0f;

	FVector RightVector = OwnerCharacter->GetActorRightVector();
	RightVector.Z = 0.0f;

	if (PlanarVelocity.Normalize() && ForwardVector.Normalize() && RightVector.Normalize())
	{
		ForwardSpeed = FVector::DotProduct(ForwardVector, PlanarVelocity) * GroundSpeed;
		StrafeSpeed = FVector::DotProduct(RightVector, PlanarVelocity) * GroundSpeed;
	}
	else
	{
		ForwardSpeed = 0.0f;
		StrafeSpeed = 0.0f;
	}

	const bool bHasAcceleration = MovementComponent
		? !MovementComponent->GetCurrentAcceleration().IsNearlyZero()
		: false;
	bShouldMove = GroundSpeed > 3.0f && bHasAcceleration;

	bIsDead = FTCharacter ? FTCharacter->IsDead() : false;
	bIsImmobilized = FTCharacter ? FTCharacter->IsImmobilized() : false;
	ImmobilizedTag = FTCharacter ? FTCharacter->GetActiveImmobilizePoseTag() : FGameplayTag();
}
