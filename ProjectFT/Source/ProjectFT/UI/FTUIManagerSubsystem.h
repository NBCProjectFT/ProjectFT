#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTUIManagerSubsystem.generated.h"

class UFTHUDViewModel;
class UFTInventoryViewModel;
class UFTCraftingViewModel;
class UFTQuestViewModel;
class UFTSettlementViewModel;
class UFTInventoryWidget;
class UFTMainMenuWidget;
class UFTCountdownEscapeWidget;

UCLASS()
class PROJECTFT_API UFTUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|UI")
	TObjectPtr<UFTHUDViewModel> HUDViewModel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FT|UI")
	TObjectPtr<UFTInventoryViewModel> InventoryViewModel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FT|UI")
	TObjectPtr<UFTCraftingViewModel> CraftingViewModel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FT|UI")
	TObjectPtr<UFTQuestViewModel> QuestViewModel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FT|UI")
	TObjectPtr<UFTSettlementViewModel> SettlementViewModel = nullptr;

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowHUD();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideMainMenu(bool bKeepMouseCursor = false);
	
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowCountdownEscape();
	
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideCountdownEscape();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void SetCountdownEscapeRemainingTime(float RemainingTime);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowInventory();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideInventory();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowCrafting();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowStorage();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowQuestBoard();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowFailScreen();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowSettlementScreen();

private:
	APlayerController* GetPrimaryPlayerController() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryWidget> InventoryWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTMainMenuWidget> MainMenuWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTCountdownEscapeWidget> CountdownEscapeWidget = nullptr;
};
