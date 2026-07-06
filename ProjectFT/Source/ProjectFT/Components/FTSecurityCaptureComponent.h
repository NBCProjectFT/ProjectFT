#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityCaptureComponent.generated.h"

/**
 * Temporary compatibility bridge for the legacy STT_TryCaptureTarget asset.
 * Capture ownership is managed by UFTCaptureEscapeComponent and UFTGA_Grab.
 */
UCLASS(meta = (DeprecationMessage = "Use FT Send Gameplay Event with Event.Grab."))
class PROJECTFT_API UFTSecurityCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCaptureComponent();

	/** Forwards the legacy capture request to the security actor's GAS Event.Grab trigger. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Capture", meta = (DeprecatedFunction, DeprecationMessage = "Use FT Send Gameplay Event with Event.Grab."))
	bool TryCaptureTarget(AActor* SecurityActor, AActor* TargetActor, FVector CaptureLocation);
};
