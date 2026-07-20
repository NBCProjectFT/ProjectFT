// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTProgressWidget.generated.h"

class UBorder;
class UFTCaptureEscapeComponent;
class UFTInteractionComponent;
class UProgressBar;
class UTextBlock;
class UWidget;

/**
 * 채널형 상호작용 진행도와 타이밍 스킬체크 UI를 표시한다.
 * 스타일과 레이아웃 기본값은 WBP_Progress에서 관리하고, 이 클래스는 런타임 값만 반영한다.
 */
UCLASS()
class PROJECTFT_API UFTProgressWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ChanneledProgressBar;

	// WBP_Progress에서 타이밍 바 트랙으로 사용한다. 기존 이름 호환 때문에 유지한다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> EscapeProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TimingBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> B_TimingSuccessZone;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_TimingCursor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_TimingArrows;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_SpaceKey;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ProgressValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ProgressPlusValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ProgressTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ProgressLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_StruggleHint;

	UPROPERTY(Transient)
	TObjectPtr<UFTInteractionComponent> CachedInteractionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UFTCaptureEscapeComponent> CachedCaptureEscapeComponent;

	enum class EFTProgressWidgetModeType : uint8
	{
		None,
		ShelfSearch,
		Escape
	};

	EFTProgressWidgetModeType CurrentMode = EFTProgressWidgetModeType::None;
	int32 LastHandledRewardSerial = 0;
	float PlusValueAnimationTime = 0.0f;
	FVector2D PlusValueBasePosition = FVector2D::ZeroVector;
	bool bHasPlusValueBasePosition = false;

	UFTInteractionComponent* ResolveInteractionComponent();
	UFTCaptureEscapeComponent* ResolveCaptureEscapeComponent();
	void UpdateChannelProgress(UFTInteractionComponent* InteractionComponent);
	void UpdateCaptureEscapeProgress(UFTCaptureEscapeComponent* CaptureEscapeComponent);
	void UpdateTimingWidgets(UFTInteractionComponent* InteractionComponent);
	void SetTimingWidgetsVisible(bool bVisible);
	void SetWidgetOptionalVisibility(UWidget* Widget, bool bVisible) const;
	void SetProgressMode(EFTProgressWidgetModeType NewMode);
	void StartProgressPlusFeedback(float ProgressBonus);
	void UpdateProgressPlusFeedback(float DeltaTime);
	void UpdateStruggleHint();
};
