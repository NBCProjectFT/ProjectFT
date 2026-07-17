#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FTReportMessageLibrary.generated.h"

UCLASS()
class PROJECTFT_API UFTReportMessageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 신고 Payload를 생성하고 지정한 Gameplay Message 채널로 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Report", meta = (WorldContext = "WorldContextObject"))
	static bool BroadcastNPCReportMessage(
		const UObject* WorldContextObject,
		FGameplayTag Channel,
		AActor* ReporterActor,
		AActor* TargetActor,
		FVector ReportLocation,
		float ReportAmount,
		float ReportProgress);

	/** 신고 완료 메시지를 공통 Payload 형식으로 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Report", meta = (WorldContext = "WorldContextObject"))
	static bool BroadcastNPCReportCompleted(
		const UObject* WorldContextObject,
		AActor* ReporterActor,
		AActor* TargetActor,
		FVector ReportLocation,
		float ReportAmount);
};
