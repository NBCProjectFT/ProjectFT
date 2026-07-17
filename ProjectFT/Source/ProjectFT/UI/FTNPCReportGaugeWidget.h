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

	/** 신고 완료 아이콘을 몇초동안 보여줄지 정합니다. 기본값은 3초로 지정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportIconDisplayDuration = 3.0f;

private:
	UPROPERTY()
	TObjectPtr<AActor> ReportOwnerActor;

	FGameplayMessageListenerHandle ReportGaugeChangedListenerHandle;
	
	/** 3초 뒤 아이콘을 숨기는 타이머를 관리하기 위한 핸들입니다. */
	FTimerHandle ReportIconTimerHandle;

	void OnReportGaugeChanged(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void UpdateReportProgress(float ReportProgress);
	void SetReportCompleted(bool bCompleted);
	
	/** 타이머가 끝났을때 호출할 함수입니다. 아이콘을 숨깁니다. */
	void HideReportIcon();
};
