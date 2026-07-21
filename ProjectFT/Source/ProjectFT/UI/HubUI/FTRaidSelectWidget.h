#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTRaidSelectWidget.generated.h"

class UFTRaidSelectViewModel;

UCLASS(Blueprintable)
class PROJECTFT_API UFTRaidSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRaidSelect(UFTRaidSelectViewModel* InViewModel);

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	void CloseRaidSelect();

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Raid", meta = (DisplayName = "On Raid ViewModel Changed"))
	void BP_OnRaidViewModelChanged(UFTRaidSelectViewModel* RaidViewModel);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "FT|Raid")
	TObjectPtr<UFTRaidSelectViewModel> ViewModel;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void RefreshFromViewModel();
};
