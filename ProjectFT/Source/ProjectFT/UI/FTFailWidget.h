#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTFailWidget.generated.h"

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
};
