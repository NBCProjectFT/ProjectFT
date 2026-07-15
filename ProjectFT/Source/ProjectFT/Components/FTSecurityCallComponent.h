#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityCallComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityCallComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCallComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Call", meta = (ClampMin = "0.0"))
	float SecurityCallDuration = 5.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Call")
	float SecurityCallProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Call")
	bool bSecurityCallCompleted = false;

	void StartSecurityCall(AActor* InTargetActor);
	void StopSecurityCall();
	void TickSecurityCall(float DeltaTime, bool bCanCharge, const FVector& LastKnownLocation, bool bHasSeenTarget);

private:
	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	void BroadcastGaugeChanged(const FVector& LastKnownLocation, bool bHasSeenTarget) const;
	void CompleteSecurityCall(const FVector& LastKnownLocation, bool bHasSeenTarget);
	AActor* GetSecurityActor() const;
};
