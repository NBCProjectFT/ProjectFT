#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FTAIControllerBase.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class PROJECTFT_API AFTAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	AFTAIControllerBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|AI|Debug")
	bool bDrawSightDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|AI|Debug")
	bool bLogPerceptionDebug = false;

protected:
	void ConfigureSight(
		UAIPerceptionComponent* InPerceptionComponent,
		UAISenseConfig_Sight* InSightConfig,
		float SightRadius,
		float PeripheralVisionAngleDegrees,
		float MaxAge);

	void RefreshSightConfig(
		UAIPerceptionComponent* InPerceptionComponent,
		UAISenseConfig_Sight* InSightConfig) const;

	bool IsActorVisibleBySight(AActor* Actor, const UAISenseConfig_Sight* InSightConfig) const;

	void DrawFlatSightDebug(
		const UAISenseConfig_Sight* InSightConfig,
		FColor Color,
		float Thickness) const;

	void DrawFlatSectorDebug(
		float Radius,
		float HalfAngleDegrees,
		FColor Color,
		float Thickness) const;
};
