#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityCoordinationComponent.generated.h"

class AFTSecurityAIController;

/**
 * Selects one attack leader and assigns stable encirclement slots to the remaining security AI.
 * StateTree uses bIsAttackLeader for Approach and EncircleSlotLocation for Encircle movement.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityCoordinationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCoordinationComponent();
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Registers a security controller as an attack leader candidate. */
	void RegisterSecurityController(AFTSecurityAIController* SecurityController);

	/** Removes a security controller from attack leader selection. */
	void UnregisterSecurityController(AFTSecurityAIController* SecurityController);

	/** Returns the security controller currently allowed to approach the target. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Coordination")
	AFTSecurityAIController* GetAttackLeader() const;

protected:
	/** A new candidate must be this much closer before replacing a valid leader. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.0"))
	float LeaderSwitchDistanceAdvantage = 100.0f;

	/** Interval between attack leader evaluations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.05"))
	float SelectionInterval = 0.2f;

	/** Distance from the target used when generating encirclement slots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "100.0"))
	float EncircleRadius = 550.0f;

	/** Extent used to project generated slots onto NavMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination")
	FVector EncircleNavProjectionExtent = FVector(200.0f, 200.0f, 300.0f);

private:
	TSet<TWeakObjectPtr<AFTSecurityAIController>> RegisteredSecurityControllers;
	TWeakObjectPtr<AFTSecurityAIController> AttackLeader;

	bool IsEligibleAttackLeader(const AFTSecurityAIController* SecurityController) const;
	void UpdateAttackLeader();
	void UpdateEncircleSlots();
	void SetAttackLeader(AFTSecurityAIController* NewAttackLeader);
	void RemoveInvalidControllers();
};
