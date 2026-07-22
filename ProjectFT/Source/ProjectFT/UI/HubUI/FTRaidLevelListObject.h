#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "FTRaidLevelListObject.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class PROJECTFT_API UFTRaidLevelListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FFTRaidEntranceOption& InOption, int32 InOptionIndex);

	const FFTRaidEntranceOption& GetOption() const { return Option; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid Level")
	int32 GetOptionIndex() const { return OptionIndex; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid Level")
	FText GetDisplayName() const { return Option.DisplayName; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid Level")
	FText GetDescription() const { return Option.Description; }

	UFUNCTION(BlueprintPure, Category = "FT|Raid Level")
	TSoftObjectPtr<UTexture2D> GetLevelPreview() const { return Option.PreviewImage; }

private:
	UPROPERTY(Transient)
	FFTRaidEntranceOption Option;

	int32 OptionIndex = INDEX_NONE;
};
