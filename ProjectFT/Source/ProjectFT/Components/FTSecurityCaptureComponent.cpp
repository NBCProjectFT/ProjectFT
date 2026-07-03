#include "FTSecurityCaptureComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Interface/FTCapturable.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTSecurityCaptureComponent::UFTSecurityCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFTSecurityCaptureComponent::TryCaptureTarget(
	AActor* SecurityActor,
	AActor* TargetActor,
	FVector CaptureLocation)
{
	if (const AController* SecurityController = Cast<AController>(SecurityActor))
	{
		SecurityActor = SecurityController->GetPawn();
	}

	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(SecurityActor) || !IsValid(TargetActor))
	{
		return false;
	}

	if (IsTargetCaptured())
	{
		return CaptorActor.Get() == SecurityActor && CapturedTargetActor.Get() == TargetActor;
	}

	IFTCapturable* CapturableTarget = Cast<IFTCapturable>(TargetActor);
	if (!CapturableTarget || !CapturableTarget->CanBeCapturedBy(SecurityActor))
	{
		return false;
	}

	CaptorActor = SecurityActor;
	CapturedTargetActor = TargetActor;
	CapturableTarget->BeginCapture(SecurityActor);

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = SecurityActor;
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = CaptureLocation;
	Payload.ReportAmount = 0.0f;
	Payload.ReportProgress = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityTargetCaptured, Payload);
	UE_LOG(LogFTSecurity, Log, TEXT("Security '%s' captured target '%s'"), *GetNameSafe(SecurityActor), *GetNameSafe(TargetActor));
	return true;
}

bool UFTSecurityCaptureComponent::RequestTargetEscape(AActor* TargetActor, FVector EscapeLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsTargetCaptured() || CapturedTargetActor.Get() != TargetActor)
	{
		return false;
	}

	AActor* ReleasedCaptorActor = CaptorActor.Get();
	AActor* ReleasedTargetActor = CapturedTargetActor.Get();
	CaptorActor.Reset();
	CapturedTargetActor.Reset();

	if (IFTCapturable* CapturableTarget = Cast<IFTCapturable>(ReleasedTargetActor))
	{
		CapturableTarget->EndCapture(ReleasedCaptorActor);
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = ReleasedCaptorActor;
	Payload.TargetActor = ReleasedTargetActor;
	Payload.ReportLocation = EscapeLocation;
	Payload.ReportAmount = 0.0f;
	Payload.ReportProgress = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityTargetEscaped, Payload);
	UE_LOG(LogFTSecurity, Log, TEXT("Target '%s' escaped from security '%s'"), *GetNameSafe(ReleasedTargetActor), *GetNameSafe(ReleasedCaptorActor));
	return true;
}

bool UFTSecurityCaptureComponent::IsTargetCaptured() const
{
	return CaptorActor.IsValid() && CapturedTargetActor.IsValid();
}

AActor* UFTSecurityCaptureComponent::GetCaptorActor() const
{
	return CaptorActor.Get();
}

AActor* UFTSecurityCaptureComponent::GetCapturedTargetActor() const
{
	return CapturedTargetActor.Get();
}
