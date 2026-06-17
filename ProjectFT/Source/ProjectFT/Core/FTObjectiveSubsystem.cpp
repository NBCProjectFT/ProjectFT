#include "FTObjectiveSubsystem.h"

#include "../Message/FTGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

bool UFTObjectiveSubsystem::IsObjectiveCompleted() const
{
	for (const FName& RequiredItem : RequiredItems)
	{
		if (!PickedUpRequiredItems.Contains(RequiredItem))
		{
			return false;
		}
	}

	return true;
}

void UFTObjectiveSubsystem::NotifyItemPickedUp(FName ItemId)
{
	if (RequiredItems.Contains(ItemId))
	{
		PickedUpRequiredItems.Add(ItemId);
	}

	if (IsObjectiveCompleted())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		FFTMessagePayloadStruct Payload;
		Payload.ItemId = ItemId;
		Payload.QuestId = CurrentQuestId;
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ObjectiveCompleted, Payload);
	}
}

void UFTObjectiveSubsystem::NotifyEscapeReached()
{
}
