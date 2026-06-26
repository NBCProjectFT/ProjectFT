#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubTerminal.generated.h"

class AFTHubQuestBoard;
class AFTHubShop;
class UFTHubMainWidget;
class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API AFTHubTerminal : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubTerminal();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Hub|UI")
	void CloseHubWidget();

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Hub")
	AFTHubQuestBoard* HubQuestBoard;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Hub")
	AFTHubShop* HubShop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hub|UI")
	TSubclassOf<UFTHubMainWidget> HubMainWidgetClass;

private:
	void OpenHubWidget(AActor* Interactor);
	UFTInventoryComponent* FindPlayerInventory(AActor* Interactor) const;

	UPROPERTY(Transient)
	UFTHubMainWidget* HubMainWidget;
};
