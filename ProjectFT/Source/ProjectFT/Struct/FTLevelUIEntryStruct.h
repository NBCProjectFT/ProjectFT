#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "FTLevelUIEntryStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTLevelUIEntryStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level UI")
	FGameplayTag UITag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level UI")
	TSoftClassPtr<UUserWidget> WidgetClass;
};
