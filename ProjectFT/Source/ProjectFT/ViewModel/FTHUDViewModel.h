#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTCrosshairStateStruct.h"
#include "FTHUDViewModel.generated.h"

class APawn;
class UAbilitySystemComponent;
class UTexture2D;
struct FOnAttributeChangeData;

UCLASS(BlueprintType)
class PROJECTFT_API UFTItemSlotDataObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|HUD|ItemSlot")
	bool bSelected = false;
};

UCLASS(BlueprintType)
class PROJECTFT_API UFTHUDViewModel : public UObject
{
	GENERATED_BODY()
public:
	UFTHUDViewModel();
	~UFTHUDViewModel();
private:
	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float HP = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float Stamina = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float CurrentWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float MaxWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float ReportGauge = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	FText ObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float TargetHPPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|HUD", meta=(AllowPrivateAccess = "true"))
	float TargetStaminaPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|HUD|ItemSlot", meta=(AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UFTItemSlotDataObject>> ItemSlotObjects;

	UPROPERTY(BlueprintReadOnly, Category = "FT|HUD|Crosshair", meta=(AllowPrivateAccess = "true"))
	FTCrosshairStateStruct CrosshairState;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;

public:
	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void InitializeFromPlayer(APawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void ClearPlayerBinding();

	UFUNCTION(BlueprintPure, Category = "FT|HUD")
	bool IsPlayerBound() const;

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void RefreshPlayerAttributes();

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void NotifyChanged();

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetHPPercent(float NewHPPercent);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetHPValue(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetStamina(float CurrentStamina, float MaxStamina);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetWeight(float NewCurrentWeight, float NewMaxWeight);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetReportGauge(float NewReportGauge);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void SetObjectiveText(const FText& NewObjectiveText);

	UFUNCTION(BlueprintPure, Category = "FT|HUD")
	float GetTargetHPPercent() const;

	UFUNCTION(BlueprintPure, Category = "FT|HUD")
	float GetTargetStaminaPercent() const;

	UFUNCTION(BlueprintCallable, Category = "FT|HUD|Crosshair")
	void SetCrosshairState(const FTCrosshairStateStruct& NewCrosshairState);

	UFUNCTION(BlueprintPure, Category = "FT|HUD|Crosshair")
	const FTCrosshairStateStruct& GetCrosshairState() const;

public:
	void TestCode();
	
private:
	UFTItemSlotDataObject* GetOrCreateItemSlot(int32 SlotIndex);
	float NormalizePercent(float Value) const;
	void InitializeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent);
	void RefreshAttributeValues();
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
	void OnStaminaAttributeChanged(const FOnAttributeChangeData& Data);
	void OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data);
};
