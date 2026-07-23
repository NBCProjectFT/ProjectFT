#include "FTBGMSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "FTLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Struct/FTFlowLevelRouteStruct.h"
#include "Sound/SoundBase.h"

void UFTBGMSubsystem::Deinitialize()
{
	if (PendingBGMHandle.IsValid())
	{
		PendingBGMHandle->CancelHandle();
		PendingBGMHandle.Reset();
	}

	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->Stop();
		CurrentBGMComponent->DestroyComponent();
		CurrentBGMComponent = nullptr;
	}

	if (NextBGMComponent)
	{
		NextBGMComponent->Stop();
		NextBGMComponent->DestroyComponent();
		NextBGMComponent = nullptr;
	}

	CurrentBGM = nullptr;
	CurrentBGMPath.Reset();

	Super::Deinitialize();
}

void UFTBGMSubsystem::ApplyLevelBGM(const FFTFlowLevelRouteStruct& Route)
{
	if (Route.bKeepCurrentBGM)
	{
		UE_LOG(LogFTAudio, Log, TEXT("Keeping current BGM. State=%d Level=%s"),
			static_cast<uint8>(Route.State),
			*Route.Level.ToSoftObjectPath().ToString());
		return;
	}

	if (Route.BGM.IsNull())
	{
		UE_LOG(LogFTAudio, Log, TEXT("No BGM set for route. Fading out current BGM. State=%d Level=%s"),
			static_cast<uint8>(Route.State),
			*Route.Level.ToSoftObjectPath().ToString());
		StopBGM(Route.BGMFadeOutTime);
		return;
	}

	PlayBGM(Route.BGM, Route.BGMFadeInTime, Route.BGMFadeOutTime, Route.BGMVolume);
}

void UFTBGMSubsystem::PlayBGM(TSoftObjectPtr<USoundBase> BGM, float FadeInTime, float FadeOutTime, float Volume)
{
	if (BGM.IsNull())
	{
		StopBGM(FadeOutTime);
		return;
	}

	const FSoftObjectPath BGMPath = BGM.ToSoftObjectPath();
	if (CurrentBGMPath == BGMPath && CurrentBGMComponent)
	{
		CurrentBGMComponent->AdjustVolume(FadeInTime, Volume);
		UE_LOG(LogFTAudio, Log, TEXT("Same BGM requested. Adjusting volume only. BGM=%s"), *BGMPath.ToString());
		return;
	}

	if (USoundBase* LoadedBGM = BGM.Get())
	{
		PlayLoadedBGM(LoadedBGM, BGMPath, FadeInTime, FadeOutTime, Volume);
		return;
	}

	if (PendingBGMHandle.IsValid())
	{
		PendingBGMHandle->CancelHandle();
		PendingBGMHandle.Reset();
	}

	UE_LOG(LogFTAudio, Log, TEXT("Loading BGM asynchronously. BGM=%s"), *BGMPath.ToString());
	PendingBGMHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		BGMPath,
		FStreamableDelegate::CreateUObject(this, &ThisClass::PlayLoadedBGM, BGM.Get(), BGMPath, FadeInTime, FadeOutTime, Volume));
}

void UFTBGMSubsystem::StopBGM(float FadeOutTime)
{
	if (PendingBGMHandle.IsValid())
	{
		PendingBGMHandle->CancelHandle();
		PendingBGMHandle.Reset();
	}

	if (!CurrentBGMComponent)
	{
		CurrentBGM = nullptr;
		CurrentBGMPath.Reset();
		return;
	}

	UE_LOG(LogFTAudio, Log, TEXT("Stopping BGM. BGM=%s FadeOut=%.2f"), *CurrentBGMPath.ToString(), FadeOutTime);
	CurrentBGMComponent->FadeOut(FMath::Max(0.0f, FadeOutTime), 0.0f);
	CleanupAudioComponent(CurrentBGMComponent, FadeOutTime);
	CurrentBGMComponent = nullptr;
	CurrentBGM = nullptr;
	CurrentBGMPath.Reset();
}

void UFTBGMSubsystem::PlayLoadedBGM(USoundBase* BGM, FSoftObjectPath BGMPath, float FadeInTime, float FadeOutTime, float Volume)
{
	if (!BGM)
	{
		BGM = Cast<USoundBase>(BGMPath.ResolveObject());
	}

	if (!BGM)
	{
		UE_LOG(LogFTAudio, Warning, TEXT("BGM failed to load. BGM=%s"), *BGMPath.ToString());
		return;
	}

	if (CurrentBGMPath == BGMPath && CurrentBGMComponent)
	{
		CurrentBGMComponent->AdjustVolume(FadeInTime, Volume);
		return;
	}

	UAudioComponent* PreviousComponent = CurrentBGMComponent;
	if (PreviousComponent)
	{
		PreviousComponent->FadeOut(FMath::Max(0.0f, FadeOutTime), 0.0f);
		CleanupAudioComponent(PreviousComponent, FadeOutTime);
	}

	NextBGMComponent = UGameplayStatics::CreateSound2D(this, BGM, Volume, 1.0f, 0.0f, nullptr, true, false);
	if (!NextBGMComponent)
	{
		UE_LOG(LogFTAudio, Warning, TEXT("BGM audio component creation failed. BGM=%s"), *BGMPath.ToString());
		return;
	}

	NextBGMComponent->FadeIn(FMath::Max(0.0f, FadeInTime), Volume);
	CurrentBGMComponent = NextBGMComponent;
	NextBGMComponent = nullptr;
	CurrentBGM = BGM;
	CurrentBGMPath = BGMPath;
	PendingBGMHandle.Reset();

	UE_LOG(LogFTAudio, Log, TEXT("Playing BGM. BGM=%s FadeIn=%.2f FadeOut=%.2f Volume=%.2f"),
		*BGMPath.ToString(),
		FadeInTime,
		FadeOutTime,
		Volume);
}

void UFTBGMSubsystem::CleanupAudioComponent(UAudioComponent* AudioComponent, float Delay)
{
	if (!AudioComponent)
	{
		return;
	}

	const TWeakObjectPtr<UAudioComponent> WeakAudioComponent(AudioComponent);
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateLambda([WeakAudioComponent]()
			{
				if (UAudioComponent* Component = WeakAudioComponent.Get())
				{
					Component->Stop();
					Component->DestroyComponent();
				}
			}),
			FMath::Max(0.01f, Delay),
			false);
		return;
	}

	AudioComponent->Stop();
	AudioComponent->DestroyComponent();
}
