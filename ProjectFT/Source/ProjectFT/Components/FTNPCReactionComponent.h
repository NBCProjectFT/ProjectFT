#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTNPCReactionComponent.generated.h"

class AFTNPCAIController;
class UAnimSequenceBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTNPCReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTNPCReactionComponent();

	/** 손님NPC를 마지막으로 위협한 대상이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	TObjectPtr<AActor> LastThreatActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction|Animation")
	TArray<TObjectPtr<UAnimSequenceBase>> SurprisedAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction|Animation")
	TArray<TObjectPtr<UAnimSequenceBase>> ReactingAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction|Animation", meta = (ClampMin = "0.0"))
	float ReactionAnimationPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction|Animation")
	FName ReactionSlotName = TEXT("DefaultSlot");

	void SetLastThreatActor(AActor* ThreatActor);
	void EnterPanic();
	void HandleImmobilizedStateChanged(bool bImmobilized);
	bool PickFleeLocationFrom(AActor* ThreatActor);
	bool RequestFleeFromTarget();
	void FinishFlee();
	void TickReaction();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction|Animation")
	bool PlaySurprisedMontage();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction|Animation")
	bool PlayReactingMontage();

private:
	bool bPanicAfterImmobilized = false;
	float PreFleeMoveSpeed = 0.0f;
	bool bHasPreFleeMoveSpeed = false;

	AFTNPCAIController* GetNPCAIController() const;
	bool IsPlayerActor(const AActor* Actor) const;
	void StartFleeMovement();
	void StopFleeMovement();
	bool PlayRandomReactionAnimation(const TArray<TObjectPtr<UAnimSequenceBase>>& Animations);
};
