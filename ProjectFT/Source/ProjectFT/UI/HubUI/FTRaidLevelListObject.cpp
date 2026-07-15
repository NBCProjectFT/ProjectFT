#include "FTRaidLevelListObject.h"

void UFTRaidLevelListObject::Initialize(
	const FFTRaidEntranceOption& InOption,
	const int32 InOptionIndex,
	UTexture2D* InLevelPreview)
{
	Option = InOption;
	OptionIndex = InOptionIndex;
	LevelPreview = InLevelPreview;
}
