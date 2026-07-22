#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubQuestPanelWidget.generated.h"

class UFTObjectiveSubsystem;
class UFTInventoryComponent;
class UFTQuestViewModel;

UCLASS(Blueprintable)
class PROJECTFT_API UFTHubQuestPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Quest")
	void InitializeQuestPanel(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Quest", meta = (DisplayName = "On Quest ViewModel Changed"))
	void BP_OnQuestViewModelChanged(UFTQuestViewModel* QuestViewModel);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Quest")
	TObjectPtr<UFTQuestViewModel> ViewModel;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void RefreshFromViewModel();
};
