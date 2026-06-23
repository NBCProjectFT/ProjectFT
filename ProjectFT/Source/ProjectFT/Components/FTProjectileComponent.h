#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Struct/FTItemActionDefinition.h"
#include "FTProjectileComponent.generated.h"

class AFTProjectileActor;
class APawn;
class UAbilitySystemComponent;

UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTProjectileComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTProjectileComponent();

	AFTProjectileActor* SpawnProjectile(
		const FFTItemActionDefinition& ActionDefinition,
		const FTransform& SpawnTransform,
		UAbilitySystemComponent* SourceAbilitySystem,
		float Damage,
		FVector LaunchDirection,
		APawn* InstigatorPawn) const;
};
