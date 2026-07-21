#include "FTNPCAnimInstance.h"

#include "ProjectFT/NPC/FTNPCCharacter.h"
#include "ProjectFT/NPC/FTNPCAIController.h"

void UFTNPCAnimInstance::CacheOwnerReferences()
{
	Super::CacheOwnerReferences();

	NPCCharacter = Cast<AFTNPCCharacter>(OwnerCharacter);
	NPCAIController = NPCCharacter ? Cast<AFTNPCAIController>(NPCCharacter->GetController()) : nullptr;
}

void UFTNPCAnimInstance::UpdateCharacterState(float DeltaSeconds)
{
	Super::UpdateCharacterState(DeltaSeconds);

	if (!NPCCharacter)
	{
		bIsShopping = false;
		bIsReporting = false;
		bIsPanicking = false;
		bIsFleeing = false;
		bIsKnockedOut = false;
		bObservedThreat = false;
		ReportProgress = 0.0f;
		return;
	}

	if (!NPCAIController)
	{
		NPCAIController = Cast<AFTNPCAIController>(NPCCharacter->GetController());
	}

	bIsFleeing = NPCCharacter->bIsFleeing;

	if (!NPCAIController)
	{
		bIsShopping = false;
		bIsReporting = false;
		bIsPanicking = false;
		bIsKnockedOut = false;
		bObservedThreat = false;
		ReportProgress = 0.0f;
		return;
	}

	bIsShopping = NPCAIController->bHasShoppingTarget && !NPCAIController->bPanicRequested && !NPCAIController->bFleeRequested;
	ReportProgress = NPCAIController->CurrentReportProgress;
	bIsReporting = ReportProgress > 0.0f && !NPCAIController->bReportCompleted && !NPCAIController->bReportCancelled;
	bIsPanicking = NPCAIController->bPanicRequested;
	bIsKnockedOut = NPCAIController->bKnockedOut;
	bObservedThreat = NPCAIController->bObservedShelfDamaged || NPCAIController->bObservedAssault;
	bIsStunned = NPCAIController->bIsStunned;
}
