#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTNPCReportGaugeWidget.generated.h"

class UProgressBar;
class UImage;
struct FFTNPCReportPayloadStruct;

UCLASS()
class PROJECTFT_API UFTNPCReportGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetReportOwnerActor(AActor* InReportOwnerActor);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ReportProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReportIconImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bHideWhenEmpty = true;

private:
	UPROPERTY()
	TObjectPtr<AActor> ReportOwnerActor;

	FGameplayMessageListenerHandle ReportGaugeChangedListenerHandle;

	void OnReportGaugeChanged(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void UpdateReportProgress(float ReportProgress);
	void SetReportCompleted(bool bCompleted);
};
