#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubWorkbench.generated.h"

class AFTHubStorage;
class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API AFTHubWorkbench : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubWorkbench();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Craft")
	AFTHubStorage* HubStorage;

private:
	void OpenCraftWidget(AActor* Interactor);

	UFTInventoryComponent* GetStorageInventory() const;
};
