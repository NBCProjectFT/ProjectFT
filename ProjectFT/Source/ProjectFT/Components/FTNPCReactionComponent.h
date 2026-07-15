#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTNPCReactionComponent.generated.h"

class AFTNPCAIController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTNPCReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTNPCReactionComponent();

	/** 손님NPC를 마지막으로 위협한 대상이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	TObjectPtr<AActor> LastThreatActor;

	void SetLastThreatActor(AActor* ThreatActor);
	void EnterPanic();
	void HandleImmobilizedStateChanged(bool bImmobilized);
	bool PickFleeLocationFrom(AActor* ThreatActor);
	bool RequestFleeFromTarget();
	void FinishFlee();
	void TickReaction();

private:
	bool bPanicAfterImmobilized = false;
	float PreFleeMoveSpeed = 0.0f;
	bool bHasPreFleeMoveSpeed = false;

	AFTNPCAIController* GetNPCAIController() const;
	bool IsPlayerActor(const AActor* Actor) const;
	void StartFleeMovement();
	void StopFleeMovement();
};
