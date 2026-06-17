#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTStorageWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Storage")
	void RefreshStorage();

	UFUNCTION(BlueprintCallable, Category = "FT|Storage")
	void RequestStoreItem(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "FT|Storage")
	void RequestTakeItem(FName ItemId);
};
