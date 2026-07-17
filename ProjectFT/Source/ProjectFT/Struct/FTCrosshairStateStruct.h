#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "FTCrosshairStateStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FTCrosshairStateStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	TSoftObjectPtr<UTexture2D> CenterTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	TSoftObjectPtr<UTexture2D> LeftTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	TSoftObjectPtr<UTexture2D> RightTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	TSoftObjectPtr<UTexture2D> TopTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	TSoftObjectPtr<UTexture2D> BottomTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float Spread = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float SpreadMax = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	bool bVisible = false;
};
