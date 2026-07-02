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
};
