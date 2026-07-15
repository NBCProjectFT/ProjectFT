#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Hub/FTHubRaidEntrance.h"
#include "FTRaidLevelListObject.generated.h"

class UTexture2D;

UCLASS()
class PROJECTFT_API UFTRaidLevelListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		const FFTRaidEntranceOption& InOption,
		int32 InOptionIndex,
		UTexture2D* InLevelPreview);

	const FFTRaidEntranceOption& GetOption() const { return Option; }
	int32 GetOptionIndex() const { return OptionIndex; }
	UTexture2D* GetLevelPreview() const { return LevelPreview; }

private:
	UPROPERTY(Transient)
	FFTRaidEntranceOption Option;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LevelPreview;

	int32 OptionIndex = INDEX_NONE;
};
