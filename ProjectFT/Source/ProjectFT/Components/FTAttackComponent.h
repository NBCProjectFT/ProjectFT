#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTAttackComponent.generated.h"

class APlayerController;
class UInputComponent;

/** Temporary component that forwards left-click input to the owner's equipment component. */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTAttackComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryBindInput();
	void HandleAttackPressed();

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> AttackInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> BoundController;

	FTimerHandle BindRetryTimer;
};
