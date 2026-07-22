#include "FTRaidLevelListObject.h"

void UFTRaidLevelListObject::Initialize(
	const FFTRaidEntranceOption& InOption,
	const int32 InOptionIndex)
{
	Option = InOption;
	OptionIndex = InOptionIndex;
}
