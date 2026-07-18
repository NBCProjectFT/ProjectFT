#include "FTReportMessageLibrary.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

bool UFTReportMessageLibrary::BroadcastNPCReportMessage(
	const UObject* WorldContextObject,
	FGameplayTag Channel,
	AActor* ReporterActor,
	AActor* TargetActor,
	FVector ReportLocation,
	float ReportAmount,
	float ReportProgress)
{
	if (!WorldContextObject || !Channel.IsValid() || !ReporterActor)
	{
		return false;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = ReporterActor;
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = ReportLocation;
	Payload.ReportAmount = ReportAmount;
	Payload.ReportProgress = ReportProgress;

	UGameplayMessageSubsystem::Get(WorldContextObject).BroadcastMessage(Channel, Payload);
	return true;
}

bool UFTReportMessageLibrary::BroadcastNPCReportCompleted(
	const UObject* WorldContextObject,
	AActor* ReporterActor,
	AActor* TargetActor,
	FVector ReportLocation,
	float ReportAmount)
{
	return BroadcastNPCReportMessage(
		WorldContextObject,
		TAG_FT_Event_NPCReportCompleted,
		ReporterActor,
		TargetActor,
		ReportLocation,
		ReportAmount,
		1.0f);
}
