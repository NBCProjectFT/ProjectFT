#include "FTQuestListObject.h"

void UFTQuestListObject::Initialize(const FTQuestStruct& InQuest, const bool bInCanComplete, const bool bInAccepted)
{
	Quest = InQuest;
	bCanComplete = bInCanComplete;
	bAccepted = bInAccepted;
}

const FTQuestStruct& UFTQuestListObject::GetQuest() const
{
	return Quest;
}

bool UFTQuestListObject::CanComplete() const
{
	return bCanComplete;
}

bool UFTQuestListObject::IsAccepted() const
{
	return bAccepted;
}
