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

	/**
	 * 거점 시작과 저장 복원 후 자동으로 수락할 퀘스트.
	 * 컴퓨터를 열기 전에 Active여야 하는 첫 튜토리얼 등에 사용한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FName> AutoAcceptedQuestIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Terminal", meta = (ClampMin = "0.0"))
	float CameraBlendTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Terminal")
	bool bLockPlayerMovementDuringUse = true;

private:
	void OpenHubWidget(AActor* Interactor);
	void ShowHubWidgetAfterCameraBlend();
	void EnterComputerUseMode(AActor* Interactor);
	void ExitComputerUseMode();
	void FinishComputerExitTransition();
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
	FTimerHandle ComputerExitTransitionTimerHandle;

	bool bIsInComputerUseMode = false;
	/** 카메라가 터미널 또는 플레이어로 보간 중일 때 연속 열기·닫기 입력을 차단한다. */
	bool bIsCameraTransitionInProgress = false;
};
