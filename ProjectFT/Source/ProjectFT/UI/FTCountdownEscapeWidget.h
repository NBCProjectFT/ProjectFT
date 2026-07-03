
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTCountdownEscapeWidget.generated.h"

class UTextBlock;

/**
 * 탈출 중 남은 시간을 표시하는 위젯의 C++ 베이스.
 *
 * 실제 숫자 전환 애니메이션은 WBP에서 구현한다.
 * C++은 남은 시간을 받아 CountDownText를 갱신하고, 초 단위 숫자가 바뀌는 순간을 BP 이벤트로 알려준다.
 */

UCLASS()
class PROJECTFT_API UFTCountdownEscapeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 남은 시간을 갱신한다. 초 단위 값이 바뀌면 OnCountdownSecondChanged가 호출된다.
	UFUNCTION(BlueprintCallable, Category = "FT|Escape")
	void SetRemainingTime(float NewRemainingTime);

	// 표시를 초기 상태로 되돌린다. 탈출 취소/위젯 재사용 시 호출할 수 있다.
	UFUNCTION(BlueprintCallable, Category = "FT|Escape")
	void ResetCountdown();

	// 이후 EscapeZoneActor나 UIManager와 연결할 메시지 발행 자리.
	// 지금은 실제 Broadcast를 하지 않고, 연결 위치를 명확히 남겨두는 껍데기 함수다.
	UFUNCTION(BlueprintCallable, Category = "FT|Escape")
	void RequestCountdownMessageBroadcast(float NewRemainingTime);

protected:
	virtual void NativeConstruct() override;

	// WBP에서 TextBlock 이름을 CountDownText로 맞추면 자동 바인딩된다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountDownText = nullptr;

	// 숫자가 바뀔 때 WBP에서 카운트다운 전환 애니메이션을 재생하기 위한 이벤트.
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Escape")
	void OnCountdownSecondChanged(int32 NewSecond);

	// 메시지 발행이 필요해질 때 WBP나 C++ 확장 지점으로 사용할 이벤트.
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Escape")
	void OnCountdownMessageRequested(float NewRemainingTime);

private:
	void UpdateCountdownText(int32 DisplaySecond);

	float RemainingTime = 0.0f;
	int32 LastDisplayedSecond = INDEX_NONE;
};
