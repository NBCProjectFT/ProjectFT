#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubStorageWidget.generated.h"

class AFTHubStorage;
class UFTHubStorageViewModel;
class UFTInventoryComponent;

/**
 * Thin C++ bridge for the Hub Storage Widget Blueprint.
 * Storage rules and data live in the ViewModel/Subsystem; presentation and input live in Blueprint.
 */
UCLASS()
class PROJECTFT_API UFTHubStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Storage")
	void InitializeStorageWidget(
		AFTHubStorage* InHubStorage,
		UFTInventoryComponent* InPlayerInventory,
		UFTHubStorageViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "Hub|Storage")
	UFTHubStorageViewModel* GetStorageViewModel() const { return ViewModel; }

	UFUNCTION(BlueprintCallable, Category = "Hub|Storage")
	void CloseStorage();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Storage", meta = (DisplayName = "On Storage ViewModel Changed"))
	void BP_OnStorageViewModelChanged(UFTHubStorageViewModel* StorageViewModel);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageViewModel> ViewModel;
};
