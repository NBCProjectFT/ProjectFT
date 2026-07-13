#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FTGenerateLevelPreloadDataCommandlet.generated.h"

UCLASS()
class PROJECTFT_API UFTGenerateLevelPreloadDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFTGenerateLevelPreloadDataCommandlet();

	virtual int32 Main(const FString& Params) override;
};
