#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTNPCShoppingComponent.generated.h"

class AFTNPCAIController;
class AFTShoppingPoint;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTNPCShoppingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTNPCShoppingComponent();

	bool PickRandomShoppingTarget();
	void ReleaseShoppingTarget();
	void StartShoppingLook();
	void TickShoppingLook(float DeltaTime);
	void ClearShoppingFocusState();

private:
	UPROPERTY()
	TObjectPtr<AFTShoppingPoint> CurrentShoppingPoint;

	FVector CurrentShoppingLookLocation = FVector::ZeroVector;
	FVector DesiredShoppingLookLocation = FVector::ZeroVector;
	bool bBlendShoppingLook = false;

	AFTNPCAIController* GetNPCAIController() const;
};
