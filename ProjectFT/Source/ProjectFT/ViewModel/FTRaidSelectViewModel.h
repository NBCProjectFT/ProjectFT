#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "FTRaidSelectViewModel.generated.h"

class UFTInventoryComponent;
class UFTRaidLevelListObject;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTRaidSelectViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTRaidSelectViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AFTHubRaidEntrance* InRaidEntrance, UFTInventoryComponent* InPlayerInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	int32 GetOptionCount() const { return Options.Num(); }

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Items")
	TArray<UObject*> GetLevelObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	bool GetOption(int32 Index, FFTRaidEntranceOption& OutOption) const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	bool CanEnterOption(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	void SelectOption(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "FT|Raid|Selection")
	void SelectLevelObject(UObject* LevelObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Raid")
	bool ConfirmSelectedOption();

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	int32 GetSelectedOptionIndex() const { return SelectedOptionIndex; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Selection")
	bool HasSelectedOption() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Selection")
	UFTRaidLevelListObject* GetSelectedLevelObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Rules")
	bool CanEnterSelectedOption() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|State")
	bool IsEntryRequestInProgress() const { return bEntryRequestInProgress; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	bool IsSelectedOptionFree() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	FName GetSelectedRequiredItemID() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	FText GetSelectedRequiredItemName() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	int32 GetSelectedRequiredItemOwnedCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	int32 GetSelectedRequiredItemCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	TSoftObjectPtr<UTexture2D> GetSelectedPreviewImageSoft() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid|Data")
	TSoftObjectPtr<UTexture2D> GetSelectedRequiredItemIconSoft() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	FText GetSelectedDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "FT|Raid")
	FText GetSelectedDescription() const;

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

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LevelObjects;

	int32 SelectedOptionIndex = INDEX_NONE;
	bool bEntryRequestInProgress = false;
};
