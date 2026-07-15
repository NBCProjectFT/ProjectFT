#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTUIManagerSubsystem.generated.h"

struct FFTMessagePayloadStruct;
class UFTHUDViewModel;
class UFTInventoryViewModel;
class UFTCraftingViewModel;
class UFTQuestViewModel;
class UFTSettlementViewModel;
class UFTInventoryWidget;
class UFTMainMenuWidget;
class UFTCountdownEscapeWidget;
class UFTEscapedRaidWidget;
class UFTFailWidget;
class AFTHubStorage;
class AFTHubTerminal;
class UFTHubMainWidget;
class UFTHubStorageViewModel;
class UFTHubStorageWidget;
class UFTHubCraftWidget;
class UFTInventoryComponent;
class AFTHubRaidEntrance;
class UFTRaidSelectWidget;
class UFTRaidSelectViewModel;

UCLASS()
class PROJECTFT_API UFTUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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
	void ShowEscapedRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideEscapedRaid();

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
	void ShowCrafting(UFTInventoryComponent* PlayerInventory, UFTInventoryComponent* StorageInventory);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideCrafting();

	
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowStorage(AFTHubStorage* HubStorage, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideStorage();

	void ShowRaidSelect(AFTHubRaidEntrance* RaidEntrance, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideRaidSelect();

	void ShowHubMain(
		AFTHubTerminal* HubTerminal,
		AFTHubStorage* HubStorage,
		UFTInventoryComponent* PlayerInventory
	);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideHubMain();

	UFUNCTION(BlueprintPure, Category = "FT|UI")
	bool IsHubMainOpen() const;

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowFailScreen();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideFailScreen();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowSettlementScreen();

private:
	APlayerController* GetPrimaryPlayerController() const;
	void HandleObjectiveProgressChanged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleObjectiveCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryWidget> InventoryWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTMainMenuWidget> MainMenuWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTCountdownEscapeWidget> CountdownEscapeWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTEscapedRaidWidget> EscapedRaidWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTFailWidget> FailWidget = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTHubMainWidget> HubMainWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageViewModel> HubStorageViewModel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageWidget> HubStorageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubCraftWidget> HubCraftWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTRaidSelectWidget> RaidSelectWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTRaidSelectViewModel> RaidSelectViewModel = nullptr;

	TArray<FGameplayMessageListenerHandle> UIMessageListenerHandles;
};
