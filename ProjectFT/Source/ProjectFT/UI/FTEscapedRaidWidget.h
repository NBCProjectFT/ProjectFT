#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectFT/Enum/FTRaidResultType.h"
#include "FTEscapedRaidWidget.generated.h"

class UButton;
class UTextBlock;
class UUserWidget;

/**
 * 레이드 탈출 성공/실패 후 표시되는 공용 결과 위젯.
 * WBP_EscapedRaid 안에 WBP_Button 인스턴스를 WBP_ReturnToBaseButton_Escaped 이름으로 두고,
 * 그 내부의 FTGameButton 클릭만 C++에서 바인딩해 Flow 요청 메시지를 보낸다.
 */
UCLASS()
class PROJECTFT_API UFTEscapedRaidWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Raid Result")
	void SetRaidResult(EFTRaidResultType NewResultType);

	// 정산 정보 문구를 갱신한다. 실제 보상/획득 아이템 계산은 후속 정산 시스템에서 전달한다.
	UFUNCTION(BlueprintCallable, Category = "FT|Raid Result")
	void SetSettlementText(const FText& NewSettlementText);

	// WBP 그래프에서 직접 호출할 수도 있는 거점 복귀 요청 함수.
	UFUNCTION(BlueprintCallable, Category = "FT|Raid Result")
	void RequestReturnToBase();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Raid Result")
	void OnEscapedRaidResult();

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Raid Result")
	void OnFailedRaidResult();

	// WBP_EscapedRaid 안의 정산 텍스트 이름을 SettlementText로 맞추면 자동 바인딩된다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Raid Result")
	TObjectPtr<UTextBlock> SettlementText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FT|Raid Result")
	TObjectPtr<UTextBlock> EscapedText = nullptr;

private:
	UFUNCTION()
	void HandleReturnToBaseClicked();

	UButton* ResolveReturnToBaseButton() const;
	UButton* ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const;
	void BindReturnToBaseButton();
	void UnbindReturnToBaseButton();
	void ApplyResultText();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Raid Result", meta = (AllowPrivateAccess = "true"))
	FText EscapedResultText = FText::FromString(TEXT("탈출 성공"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Raid Result", meta = (AllowPrivateAccess = "true"))
	FText FailedResultText = FText::FromString(TEXT("탈출 실패"));

	UPROPERTY(Transient)
	EFTRaidResultType ResultType = EFTRaidResultType::Escaped;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CachedReturnToBaseButton = nullptr;
};
