#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Struct/FTCrosshairStateStruct.h"
#include "FTCrosshairComponent.generated.h"

class ACharacter;
class UFTItemDataAsset;
class UFTHitScanDataAsset;
class UFTHUDViewModel;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTCrosshairComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTCrosshairComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|Crosshair")
	void SetActiveItemData(const UFTItemDataAsset* ItemData);

	UFUNCTION(BlueprintCallable, Category = "FT|Crosshair")
	void ClearCrosshair();

	UFUNCTION(BlueprintCallable, Category = "FT|Crosshair")
	void NotifyFired();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float BaseSpread = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float DefaultSpreadMax = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float InAirTargetFactor = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float CrouchTargetFactor = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float AimTargetFactor = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair", meta = (ClampMin = "0.0"))
	float ShootingImpulse = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair")
	bool bShowDefaultCrosshair = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Crosshair")
	FTCrosshairStateStruct DefaultCrosshairData;

private:
	void ApplyDefaultCrosshair();
	void ResetCrosshairFactors();
	void PushCrosshairStateToHUD() const;
	void UpdateCrosshair(float DeltaTime);
	UFTHUDViewModel* ResolveHUDViewModel() const;

	UPROPERTY(Transient)
	TObjectPtr<const UFTHitScanDataAsset> ActiveHitScanData = nullptr;

	FTCrosshairStateStruct CrosshairState;

	float CrosshairVelocityFactor = 0.0f;
	float CrosshairInAirFactor = 0.0f;
	float CrosshairCrouchFactor = 0.0f;
	float CrosshairAimFactor = 0.0f;
	float CrosshairShootingFactor = 0.0f;
};
