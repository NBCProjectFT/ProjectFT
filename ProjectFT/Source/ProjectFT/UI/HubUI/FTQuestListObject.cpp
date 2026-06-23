#include "FTQuestListObject.h"

void UFTQuestListObject::Initialize(const FTQuestStruct& InQuest, bool bInCanComplete)
{
	Quest = InQuest;
	bCanComplete = bInCanComplete;
}

const FTQuestStruct& UFTQuestListObject::GetQuest() const
{
	return Quest;
}

bool UFTQuestListObject::CanComplete() const
{
	return bCanComplete;
}