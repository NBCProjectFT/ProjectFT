#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "FTQuestListObject.generated.h"

UCLASS()
class PROJECTFT_API UFTQuestListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTQuestStruct& InQuest, bool bInCanComplete);

	const FTQuestStruct& GetQuest() const;
	bool CanComplete() const;

private:
	FTQuestStruct Quest;
	bool bCanComplete = false;
};