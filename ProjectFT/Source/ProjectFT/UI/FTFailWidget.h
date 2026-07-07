#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTFailWidget.generated.h"

class UButton;
class UUserWidget;

UCLASS()
class PROJECTFT_API UFTFailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Fail")
	void ShowFailReason(const FText& FailReason);

	UFUNCTION(BlueprintCallable, Category = "FT|Fail")
	void ShowLostItemInfo(const TArray<FName>& LostItemIds);

	UFUNCTION(BlueprintCallable, Category = "FT|Fail")
	void RequestRetry();

	UFUNCTION(BlueprintCallable, Category = "FT|Fail")
	void RequestReturnToBase();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleReturnToBaseClicked();

	UButton* ResolveReturnToBaseButton() const;
	UButton* ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const;
	void BindReturnToBaseButton();
	void UnbindReturnToBaseButton();

	UPROPERTY(Transient)
	TObjectPtr<UButton> CachedReturnToBaseButton = nullptr;
};
