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
class AFTHubStorage;
class AFTHubWorkbench;
class UFTHubStorageViewModel;
class UFTHubStorageWidget;
class UFTHubCraftTestWidget;
class UFTInventoryComponent;

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

	// 인벤토리 열림 상태의 단일 소스. 위젯이 뷰포트에 붙어 있으면 열린 것으로 본다.
	UFUNCTION(BlueprintPure, Category = "FT|UI")
	bool IsInventoryOpen() const;

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowCrafting();

	void ShowCrafting(AFTHubWorkbench* HubWorkbench, UFTInventoryComponent* PlayerInventory, TSubclassOf<UFTHubCraftTestWidget> FallbackWidgetClass = nullptr);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideCrafting();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowStorage();

	void ShowStorage(AFTHubStorage* HubStorage, UFTInventoryComponent* PlayerInventory, TSubclassOf<UFTHubStorageWidget> FallbackWidgetClass = nullptr);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideStorage();

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

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageViewModel> HubStorageViewModel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageWidget> HubStorageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubCraftTestWidget> HubCraftWidget = nullptr;
};
