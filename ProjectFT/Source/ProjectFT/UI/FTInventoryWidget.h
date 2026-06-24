#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInventoryWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UWidgetAnimation;

UCLASS()
class PROJECTFT_API UFTInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory|Paper")
	void SetPaperMaterial(UMaterialInterface* InPaperMaterial);

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory|Paper")
	void PlayOpenPaper();

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void RefreshItemList();

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void UpdateWeight(float CurrentWeight, float MaxWeight);

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void ShowItemDetail(FName ItemId);

private:
	void StartCornerCurl();
	void UpdateCornerCurl(float DeltaTime);
	void StopPaperFlutter();
	UWidgetAnimation* FindWidgetAnimation(FName AnimationName) const;

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UImage> IMG_PaperBackground = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PaperMID = nullptr;

	FTimerHandle FlutterStopTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Inventory|Paper", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float FlutterDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Inventory|Paper", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float CornerCurlDuration = 0.56f;

	float CornerCurlElapsed = 0.0f;

	bool bUpdatingCornerCurl = false;
};
