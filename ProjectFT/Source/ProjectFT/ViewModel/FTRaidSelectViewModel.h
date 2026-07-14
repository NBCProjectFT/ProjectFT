#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "FTRaidSelectViewModel.generated.h"

class UFTInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTRaidSelectViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTRaidSelectViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AFTHubRaidEntrance* InRaidEntrance, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	int32 GetOptionCount() const { return Options.Num(); }

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	bool GetOption(int32 Index, FFTRaidEntranceOption& OutOption) const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	bool CanEnterOption(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	void SelectOption(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	bool ConfirmSelectedOption();

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	int32 GetSelectedOptionIndex() const { return SelectedOptionIndex; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	FText GetSelectedDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	FText GetSelectedEntryCostText() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	FText GetSelectedStatusText() const;

	UPROPERTY(BlueprintAssignable, Category = "FT|Raid")
	FFTRaidSelectViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	const FFTRaidEntranceOption* GetSelectedOption() const;
	void BindInventory();
	void UnbindInventory();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubRaidEntrance> RaidEntrance;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<FFTRaidEntranceOption> Options;

	int32 SelectedOptionIndex = INDEX_NONE;
	bool bEntryRequestInProgress = false;
};
