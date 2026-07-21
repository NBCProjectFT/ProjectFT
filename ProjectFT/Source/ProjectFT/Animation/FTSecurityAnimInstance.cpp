#include "FTSecurityAnimInstance.h"

#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"

void UFTSecurityAnimInstance::CacheOwnerReferences()
{
	Super::CacheOwnerReferences();

	SecurityCharacter = Cast<AFTSecurityCharacter>(OwnerCharacter);
	SecurityAIController = SecurityCharacter ? Cast<AFTSecurityAIController>(SecurityCharacter->GetController()) : nullptr;
}

void UFTSecurityAnimInstance::UpdateCharacterState(float DeltaSeconds)
{
	Super::UpdateCharacterState(DeltaSeconds);

	if (!SecurityCharacter)
	{
		bIsGrabbing = false;
		bIsCaptor = false;
		bTargetCaptured = false;
		bIsTargetCapturedByOtherSecurity = false;
		bIsReturning = false;
		bIsAttackLeader = false;
		bIsTargetInAttackRange = false;
		bSecurityChaseActive = false;
		return;
	}

	if (!SecurityAIController)
	{
		SecurityAIController = Cast<AFTSecurityAIController>(SecurityCharacter->GetController());
	}

	if (!SecurityAIController)
	{
		bIsGrabbing = false;
		bIsCaptor = false;
		bTargetCaptured = false;
		bIsTargetCapturedByOtherSecurity = false;
		bIsReturning = false;
		bIsAttackLeader = false;
		bIsTargetInAttackRange = false;
		bSecurityChaseActive = false;
		return;
	}

	bIsGrabbing = SecurityAIController->bIsGrabbing;
	bIsCaptor = SecurityAIController->bIsCaptor;
	bTargetCaptured = SecurityAIController->bTargetCaptured;
	bIsTargetCapturedByOtherSecurity = SecurityAIController->bIsTargetCapturedByOtherSecurity;
	bIsReturning = SecurityAIController->bReturning;
	bIsAttackLeader = SecurityAIController->bIsAttackLeader;
	bIsTargetInAttackRange = SecurityAIController->bIsTargetInAttackRange;
	bSecurityChaseActive = SecurityAIController->bSecurityChaseActive;
	bIsStunned = SecurityAIController->bIsStunned;
}
