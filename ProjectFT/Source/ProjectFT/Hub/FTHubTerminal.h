#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubTerminal.generated.h"

class AFTHubStorage;
class UDataTable;

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
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	UDataTable* QuestDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quest")
	AFTHubStorage* HubStorage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FName> InitialQuestIDs;

private:
	void OpenHubWidget(AActor* Interactor);
	void ConfigureObjectiveSubsystem();
};
