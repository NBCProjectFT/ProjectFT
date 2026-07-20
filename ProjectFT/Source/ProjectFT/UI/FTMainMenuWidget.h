#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTMainMenuWidget.generated.h"

class UButton;
class UUserWidget;
class UWidget;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTMainMenuRequestEvent);

/**
 * Main menu presentation widget using reusable WBP_Button children.
 *
 * Required Blueprint names:
 * - WBP_StartButton
 * - WBP_ContinueButton
 * - WBP_OptionsButton
 * - WBP_QuitButton
 * - WBP_OptionsBackButton
 * - WBP_QuitConfirmButton
 * - WBP_QuitCancelButton
 * - SW_MainMenuPanels
 * - MainPanel / OptionsPanel / QuitConfirmPanel
 * - QuitConfirmBorder (legacy fallback: Border_7)
 *
 * Every *Button entry above is a WBP_Button instance containing an inner
 * UButton named FTGameButton.
 */
UCLASS()
class PROJECTFT_API UFTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintCallable, Category = "FT|MainMenu")
	void ShowMainPanel();

	UFUNCTION(BlueprintCallable, Category = "FT|MainMenu")
	void ShowOptionsPanel();

	UFUNCTION(BlueprintCallable, Category = "FT|MainMenu")
	void ShowQuitConfirmPanel();

	/** Called by the flow/save owner after checking whether a resumable save exists. */
	UFUNCTION(BlueprintCallable, Category = "FT|MainMenu")
	void SetContinueButtonEnabled(bool bEnabled);

	UPROPERTY(BlueprintAssignable, Category = "FT|MainMenu|Request")
	FFTMainMenuRequestEvent OnContinueGameRequested;

	/** Presentation hook. Application logic should listen to the gameplay message tag. */
	UPROPERTY(BlueprintAssignable, Category = "FT|MainMenu|Request")
	FFTMainMenuRequestEvent OnQuitGameRequested;

protected:
	UFUNCTION()
	void HandleStartButtonClicked();

	UFUNCTION()
	void HandleContinueButtonClicked();

	UFUNCTION()
	void HandleOptionsButtonClicked();

	UFUNCTION()
	void HandleOptionsBackButtonClicked();

	UFUNCTION()
	void HandleQuitButtonClicked();

	UFUNCTION()
	void HandleQuitConfirmButtonClicked();

	UFUNCTION()
	void HandleQuitCancelButtonClicked();

private:
	bool BindButton(FName WrapperWidgetName, FName HandlerName, bool bRequired = false);
	UButton* ResolveWrappedButton(FName WrapperWidgetName) const;
	UButton* ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const;
	void ResolvePanels();
	void ActivatePanel(UWidget* PanelToShow);
	void HideQuitConfirm();
	bool IsQuitConfirmVisible() const;
	void QuitGame();

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> CachedMainMenuSwitcher = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CachedMainPanel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CachedOptionsPanel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CachedQuitConfirmPanel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CachedQuitConfirmBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CachedContinueButton = nullptr;
};
