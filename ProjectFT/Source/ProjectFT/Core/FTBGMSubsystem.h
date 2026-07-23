#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTBGMSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
struct FFTFlowLevelRouteStruct;
struct FStreamableHandle;

UCLASS()
class PROJECTFT_API UFTBGMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void ApplyLevelBGM(const FFTFlowLevelRouteStruct& Route);
	void PlayBGM(TSoftObjectPtr<USoundBase> BGM, float FadeInTime, float FadeOutTime, float Volume);
	void StopBGM(float FadeOutTime);

private:
	void PlayLoadedBGM(USoundBase* BGM, FSoftObjectPath BGMPath, float FadeInTime, float FadeOutTime, float Volume);
	void CleanupAudioComponent(UAudioComponent* AudioComponent, float Delay);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> NextBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGM;

	FSoftObjectPath CurrentBGMPath;
	TSharedPtr<FStreamableHandle> PendingBGMHandle;
};
