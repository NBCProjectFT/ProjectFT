#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTItemTooltipWidget.generated.h"

class UTextBlock;

/**
 * 아이템 위에 띄울 3D 툴팁 UI의 C++ 베이스 클래스
 */
UCLASS()
class PROJECTFT_API UFTItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 아이템 정보를 받아서 텍스트 컴포넌트들을 업데이트합니다.
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void SetItemInfo(const FText& Name, float Weight, int32 Cost);

protected:
	// UMG Widget의 텍스트 컴포넌트 이름과 일치해야 합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Weight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Cost;
};
