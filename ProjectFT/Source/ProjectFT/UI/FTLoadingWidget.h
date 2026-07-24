// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTLoadingWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UProgressBar;
class UWidget;

/**
 * 
 */
UCLASS()
class PROJECTFT_API UFTLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidget))
	UButton* BTN_Click;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidget))
	UTextBlock* TB_Text;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidget))
	UTextBlock* TB_ObjectName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidget))
	UTextBlock* TB_ObjectCounts;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidget))
	UProgressBar* PB_LoadingBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidgetOptional))
	UImage* Image_0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidgetOptional))
	UImage* IMG_ReadyBlackBackground;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidgetOptional))
	UWidget* SizeBox_LoadingBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Loading", meta=(AllowPrivateAccess=true, BindWidgetOptional))
	UWidget* SizeBox_LoadingText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> TB_Text_Opacity;
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	void SetPercent(float Percent) const;
	void ReadyToStart();
	bool BindOnButtonClicked(TFunction<void()> InCallback);
	void SetObjectName(const FString& ObjectName, const int32& CompleteNum, const int32& TotalNum) const;

	//람다 보관용
	TOptional<TFunction<void()>> StoredCallback;
	
	UFUNCTION()
	void HandleButtonClicked();

private:
	bool bReadyToStart = false;
};
