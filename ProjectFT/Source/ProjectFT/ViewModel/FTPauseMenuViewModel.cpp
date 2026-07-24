#include "FTPauseMenuViewModel.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Core/FTGameFlowSubsystem.h"
#include "ProjectFT/Core/FTSaveSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UFTPauseMenuViewModel::Initialize(UObject* InWorldContextObject)
{
	WorldContextObject = InWorldContextObject;
}

EFTFlowStateType UFTPauseMenuViewModel::GetCurrentFlowState() const
{
	const UGameInstance* GameInstance = ResolveGameInstance();
	const UFTGameFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTGameFlowSubsystem>() : nullptr;
	return FlowSubsystem ? FlowSubsystem->GetCurrentFlowState() : EFTFlowStateType::MainMenu;
}

bool UFTPauseMenuViewModel::IsCurrentFlowStateBase() const
{
	return GetCurrentFlowState() == EFTFlowStateType::Base;
}

bool UFTPauseMenuViewModel::ShouldShowReturnToBaseButton() const
{
	return !IsCurrentFlowStateBase();
}

void UFTPauseMenuViewModel::ApplyMasterVolume(USoundMix* MasterSoundMix, USoundClass* MasterSoundClass, float Volume) const
{
	UObject* ContextObject = WorldContextObject.Get();
	if (!ContextObject || !MasterSoundMix || !MasterSoundClass)
	{
		return;
	}

	const float ClampedVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UGameplayStatics::SetSoundMixClassOverride(ContextObject, MasterSoundMix, MasterSoundClass, ClampedVolume, 1.0f, 0.0f, true);
	UGameplayStatics::PushSoundMixModifier(ContextObject, MasterSoundMix);
}

void UFTPauseMenuViewModel::ExecuteConfirmedAction(EFTPauseMenuConfirmType ConfirmType) const
{
	if (ConfirmType == EFTPauseMenuConfirmType::ReturnToBase)
	{
		ClearPlayerInventoryForReturnToBase();
		BroadcastFlowRequest(TAG_FT_Request_Flow_ReturnToBase);
	}
	else if (ConfirmType == EFTPauseMenuConfirmType::MainMenu)
	{
		BroadcastFlowRequest(TAG_FT_Request_Flow_ReturnToMainMenu);
	}
}

void UFTPauseMenuViewModel::BroadcastFlowRequest(const FGameplayTag& RequestTag) const
{
	UObject* ContextObject = WorldContextObject.Get();
	if (!ContextObject || !RequestTag.IsValid())
	{
		return;
	}

	FFTMessagePayloadStruct Payload;
	if (const UWorld* World = ResolveWorld())
	{
		Payload.InstigatorActor = UGameplayStatics::GetPlayerPawn(World, 0);
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(ContextObject);
	MessageSubsystem.BroadcastMessage(RequestTag, Payload);
}

void UFTPauseMenuViewModel::ClearPlayerInventoryForReturnToBase() const
{
	UGameInstance* GameInstance = ResolveGameInstance();
	UFTSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTSaveSubsystem>() : nullptr;
	if (SaveSubsystem)
	{
		SaveSubsystem->ClearPlayerInventoryForRaidFailure();
	}
}

UWorld* UFTPauseMenuViewModel::ResolveWorld() const
{
	const UObject* ContextObject = WorldContextObject.Get();
	return ContextObject ? ContextObject->GetWorld() : nullptr;
}

UGameInstance* UFTPauseMenuViewModel::ResolveGameInstance() const
{
	const UWorld* World = ResolveWorld();
	return World ? World->GetGameInstance() : nullptr;
}
