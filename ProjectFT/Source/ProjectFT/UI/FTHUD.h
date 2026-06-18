#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FTHUD.generated.h"

class UUserWidget;

UCLASS()
class PROJECTFT_API AFTHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void CreateMainHUD();

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void ShowWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void HideWidget(UUserWidget* Widget);
};
