#pragma once

#include "CoreMinimal.h"

class UFTItemDataAsset;

class PROJECTFT_API FTHubItemDataResolver
{
public:
	static const UFTItemDataAsset* FindItemData(FName ItemID);
};
