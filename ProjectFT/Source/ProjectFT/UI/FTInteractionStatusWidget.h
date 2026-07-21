#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInteractionStatusWidget.generated.h"

class UFTCaptureEscapeComponent;
class UFTInteractionPromptWidget;
class UFTInteractionComponent;
class UFTProgressWidget;
class UWidget;
class UWidgetSwitcher;

UCLASS()
class PROJECTFT_API UFTInteractionStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EFTInteractionStatusModeType : uint8
	{
		None,
		Prompt,
		ShelfSearch,
		Escape
	};

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WS_InteractionState = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_47 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UFTProgressWidget> WBP_Progress = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UFTInteractionPromptWidget> WBP_InteractionPrompt = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTInteractionComponent> CachedInteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTCaptureEscapeComponent> CachedCaptureEscapeComponent = nullptr;

	EFTInteractionStatusModeType CurrentMode = EFTInteractionStatusModeType::None;

	UFTInteractionComponent* ResolveInteractionComponent();
	UFTCaptureEscapeComponent* ResolveCaptureEscapeComponent();
	void UpdatePrompt(UFTInteractionComponent* InteractionComponent);
	void SetWidgetOptionalVisibility(UWidget* Widget, bool bVisible) const;
	void SetStatusMode(EFTInteractionStatusModeType NewMode);
	void SetPromptText(const FText& PromptText);
	UWidgetSwitcher* GetInteractionSwitcher() const;
};
