#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../ViewModel/FTHUDViewModel.h"
#include "FTMainHUDWidget.generated.h"

class UListView;
class UHorizontalBox;
class UImage;
class UMaterialInstanceDynamic;
class UProgressBar;
class UTexture2D;
class UWidget;
class UUserWidget;
class UFTInteractionComponent;
class UFTInteractionStatusWidget;
class UFTShelfHealthBarWidget;

UCLASS()
class PROJECTFT_API UFTMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateHP(float NewHP);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateHPValue(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateStamina(float CurrentStamina, float MaxStemina);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateWeight(float CurrentWeight, float MaxWeight);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateReportGauge(float NewReportGauge);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateObjective(const FText& NewObjectiveText);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FT|HUD|QuickSlot")
	int32 GetQuickSlotCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FT|HUD|QuickSlot")
	UWidget* GetQuickSlotWidget(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FT|HUD|QuickSlot")
	TArray<UWidget*> GetQuickSlotWidgets() const;

private:
	UFUNCTION()
	void HandleFocusedInteractableChanged(AActor* FocusedActor);

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidget))
	TObjectPtr<UImage> IMG_HPBar = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidget))
	TObjectPtr<UImage> IMG_StaminaBar = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_QuickSlot = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_CrosshairCenter = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_CrosshairLeft = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_CrosshairRight = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_CrosshairTop = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_CrosshairBottom = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|HUD|Interaction", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFTInteractionStatusWidget> InteractionStatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|HUD|Shelf", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFTShelfHealthBarWidget> ShelfHealthBarWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|ItemSlot", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 DefaultSlotCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HPFrontInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HPBackInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Stamina", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float StaminaFrontInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Stamina", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float StaminaBackInterpSpeed = 2.0f;
	
	float CurrentHPFrontPercent = 1.0f;
	float CurrentHPBackPercent = 1.0f;
	
	float CurrentStaminaFrontPercent = 1.0f;
	float CurrentStaminaBackPercent = 1.0f;

	UPROPERTY()
	TObjectPtr<UFTHUDViewModel> HUDViewModel = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_HPBar = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_StaminaBar = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTInteractionComponent> InteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTInteractionStatusWidget> InteractionStatusWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTShelfHealthBarWidget> ShelfHealthBarWidget = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<class AFTLootShelf> FocusedShelf;
	
	void UpdateHPBars(float DeltaTime);
	void UpdateStaminaBar(float DeltaTime);
	void UpdateCrosshair();
	void ResolveHUDBarWidgets();
	void ResolveHUDViewModel();
	void ResolveInteractionPromptBinding();
	void ClearInteractionPromptBinding();
	void CreateInteractionStatusWidget();
	void RemoveInteractionStatusWidget();
	void CreateShelfHealthBarWidget();
	void RemoveShelfHealthBarWidget();
	void UpdateShelfHealthBar();
	void HideShelfHealthBar();
};
