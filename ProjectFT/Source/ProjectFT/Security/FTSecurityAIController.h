#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "FTSecurityAIController.generated.h"

UCLASS()
class PROJECTFT_API AFTSecurityAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFTSecurityAIController();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
