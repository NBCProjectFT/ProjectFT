#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMarketPanelWidget.generated.h"

class UFTMarketViewModel;
class UFTInventoryComponent;
class UFTShopSubsystem;

UCLASS(Blueprintable)
class PROJECTFT_API UFTHubMarketPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Market")
	void InitializeMarketPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Market", meta = (DisplayName = "On Market ViewModel Changed"))
	void BP_OnMarketViewModelChanged(UFTMarketViewModel* MarketViewModel);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Hub|Market")
	TObjectPtr<UFTMarketViewModel> ViewModel;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void RefreshFromViewModel();

};
