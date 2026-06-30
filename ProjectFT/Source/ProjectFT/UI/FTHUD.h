#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FTHUD.generated.h"

class UUserWidget;
class UFTMainHUDWidget;

UCLASS()
class PROJECTFT_API AFTHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void CreateMainHUD();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideWidget(UUserWidget* Widget);
	
private:
	virtual void BeginPlay() override;
	void UpdateHUDDrainTest();
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> HUDClass;

	UPROPERTY()
	TObjectPtr<UFTMainHUDWidget> MainHUDWidget = nullptr;

	FTimerHandle HUDDrainTestTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Test", meta = (AllowPrivateAccess = "true"))
	bool bEnableHUDDrainTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Test", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float HUDDrainTestInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|HUD|Test", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HUDDrainTestAmount = 3.0f;

	float TestHP = 100.0f;
	float TestStamina = 100.0f;
};
