#include "FTHubRaidEntrance.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

AFTHubRaidEntrance::AFTHubRaidEntrance()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionPrompt = FText::FromString(TEXT("Go to market"));
}

bool AFTHubRaidEntrance::Interact_Implementation(AActor* Interactor)
{
	RequestStartRaid(Interactor);
	return true;
}

FText AFTHubRaidEntrance::GetInteractionPrompt_Implementation() const
{
	return InteractionPrompt;
}

void AFTHubRaidEntrance::RequestStartRaid(AActor* InstigatorActor)
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = InstigatorActor;
	Payload.TargetActor = this;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Request_Flow_StartRaid, Payload);
}
