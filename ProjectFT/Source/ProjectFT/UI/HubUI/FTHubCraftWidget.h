#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubCraftWidget.generated.h"

class UFTCraftingViewModel;
class UFTInventoryComponent;

/** Thin C++ bridge for the Hub Craft Widget Blueprint. */
UCLASS()
class PROJECTFT_API UFTHubCraftWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeCraftWidget(
		UFTInventoryComponent* InPlayerInventory,
		UFTInventoryComponent* InStorageInventory,
		UFTCraftingViewModel* InViewModel);

	UFUNCTION(BlueprintCallable, Category = "Hub|Craft")
	void CloseCraft();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Craft", meta = (DisplayName = "On Crafting ViewModel Changed"))
	void BP_OnCraftingViewModelChanged(UFTCraftingViewModel* CraftingViewModel);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UPROPERTY(Transient)
	TObjectPtr<UFTCraftingViewModel> ViewModel;
};
