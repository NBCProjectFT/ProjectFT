#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubRaidEntrance.generated.h"

UCLASS()
class PROJECTFT_API AFTHubRaidEntrance : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubRaidEntrance();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Hub|Flow")
	void RequestStartRaid(AActor* InstigatorActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Flow")
	FText InteractionPrompt;
};
