
#pragma once

#include "CoreMinimal.h"
#include "FTItemActor.h"
#include "FTProjectileActor.generated.h"

UCLASS()
class PROJECTFT_API AFTProjectileActor : public AFTItemActor
{
	GENERATED_BODY()

public:
	AFTProjectileActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
