#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTSecurityCallGaugeWidget.generated.h"

class UImage;
class UProgressBar;
struct FFTSecurityChaseGaugePayloadStruct;

UCLASS()
class PROJECTFT_API UFTSecurityCallGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSecurityOwnerActor(AActor* InSecurityOwnerActor);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> SecurityCallProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SecurityCallIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MemoryIconImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Call")
	bool bHideWhenEmpty = true;

private:
	UPROPERTY()
	TObjectPtr<AActor> SecurityOwnerActor;

	FGameplayMessageListenerHandle SecurityCallGaugeChangedListenerHandle;

	void OnSecurityCallGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void UpdateSecurityCallProgress(float Progress);
	void SetSecurityCallCompleted(bool bCompleted);
	void UpdateMemoryIcon();
};
