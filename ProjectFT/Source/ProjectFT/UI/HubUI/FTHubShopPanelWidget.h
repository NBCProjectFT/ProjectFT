#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubShopPanelWidget.generated.h"

class UFTInventoryComponent;
class UFTShopSubsystem;
class UFTShopViewModel;

UCLASS()
class PROJECTFT_API UFTHubShopPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Shop")
	void InitializeShopPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintPure, Category = "Hub|Shop")
	UFTShopViewModel* GetShopViewModel() const { return ViewModel; }

	/**
	 * Blueprint presentation hook. Implement this in WBP_FTHubShopPanelWidget and
	 * read every visible value from the supplied ViewModel.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Shop", meta = (DisplayName = "On Shop ViewModel Changed"))
	void BP_OnShopViewModelChanged(UFTShopViewModel* ShopViewModel);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopViewModel> ViewModel;
};
