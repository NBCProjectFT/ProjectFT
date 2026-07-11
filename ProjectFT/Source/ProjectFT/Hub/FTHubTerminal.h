#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubTerminal.generated.h"

class AFTHubStorage;
class APlayerController;
class APawn;
class UCameraComponent;
class UDataTable;
class USceneComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hub|Terminal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hub|Terminal")
	TObjectPtr<UCameraComponent> TerminalCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	UDataTable* QuestDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quest")
	AFTHubStorage* HubStorage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FName> InitialQuestIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Terminal", meta = (ClampMin = "0.0"))
	float CameraBlendTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Terminal")
	bool bLockPlayerMovementDuringUse = true;

private:
	void OpenHubWidget(AActor* Interactor);
	void ShowHubWidgetAfterCameraBlend();
	void EnterComputerUseMode(AActor* Interactor);
	void ExitComputerUseMode();
	void ConfigureObjectiveSubsystem();

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> UsingPlayerController;

	UPROPERTY(Transient)
	TObjectPtr<APawn> UsingPawn;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingInteractor;

	FTimerHandle ShowHubWidgetTimerHandle;

	bool bIsInComputerUseMode = false;
};
