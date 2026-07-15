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
	void Initialize(const FTQuestStruct& InQuest, bool bInCanComplete, bool bInAccepted);

	const FTQuestStruct& GetQuest() const;
	bool CanComplete() const;
	bool IsAccepted() const;

private:
	FTQuestStruct Quest;
	bool bCanComplete = false;
	bool bAccepted = false;
};
