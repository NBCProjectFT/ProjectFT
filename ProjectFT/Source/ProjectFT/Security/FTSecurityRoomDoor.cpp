#include "FTSecurityRoomDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityResponsePayloadStruct.h"
#include "TimerManager.h"

AFTSecurityRoomDoor::AFTSecurityRoomDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(SceneRoot);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);

	ReturnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReturnPoint"));
	ReturnPoint->SetupAttachment(SceneRoot);
}

void AFTSecurityRoomDoor::BeginPlay()
{
	Super::BeginPlay();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	SecurityCalledListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCalled,
		this,
		&ThisClass::OnSecurityCalled
	);
	SecurityTargetCapturedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetCaptured,
		this,
		&ThisClass::OnSecurityTargetCaptured
	);
	ChaseEndedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityChaseEnded,
		this,
		&ThisClass::OnChaseEnded
	);
	SecurityReturnedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityReturnedToRoom,
		this,
		&ThisClass::OnSecurityReturned
	);
}

void AFTSecurityRoomDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	if (SecurityCalledListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(SecurityCalledListenerHandle);
	}
	if (SecurityTargetCapturedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(SecurityTargetCapturedListenerHandle);
	}
	if (ChaseEndedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(ChaseEndedListenerHandle);
	}
	if (SecurityReturnedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(SecurityReturnedListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AFTSecurityRoomDoor::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.TargetActor)
	{
		return;
	}

	PendingTargetActor = Payload.TargetActor;
	PendingReportLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	bResponseActive = true;
	SetDoorOpen(true);

	if (GetWorldTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	if (SpawnDelay <= 0.0f)
	{
		SpawnMissingSecurity();
		return;
	}

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&ThisClass::SpawnMissingSecurity,
		SpawnDelay,
		false
	);
}

void AFTSecurityRoomDoor::OnSecurityTargetCaptured(
	FGameplayTag Channel,
	const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.TargetActor || (PendingTargetActor && Payload.TargetActor != PendingTargetActor))
	{
		return;
	}

	bResponseActive = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security room '%s' stopped deployment because target '%s' was captured"),
		*GetName(),
		*GetNameSafe(Payload.TargetActor));
}

void AFTSecurityRoomDoor::OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	bResponseActive = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	CompactSpawnedSecurityActors();

	if (SpawnedSecurityActors.IsEmpty())
	{
		SetDoorOpen(false);
	}
}

void AFTSecurityRoomDoor::OnSecurityReturned(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload)
{
	if (Payload.SecurityRoomActor != this || !Payload.SecurityActor)
	{
		return;
	}

	for (int32 SecurityIndex = SpawnedSecurityActors.Num() - 1; SecurityIndex >= 0; --SecurityIndex)
	{
		if (!SpawnedSecurityActors[SecurityIndex].IsValid() || SpawnedSecurityActors[SecurityIndex].Get() == Payload.SecurityActor)
		{
			SpawnedSecurityActors.RemoveAtSwap(SecurityIndex);
		}
	}

	UE_LOG(LogFTSecurity, Log, TEXT("Security room '%s' despawning %s"), *GetName(), *GetNameSafe(Payload.SecurityActor));
	Payload.SecurityActor->Destroy();
	CompactSpawnedSecurityActors();

	if (bResponseActive)
	{
		SpawnMissingSecurity();
		return;
	}

	if (SpawnedSecurityActors.IsEmpty())
	{
		SetDoorOpen(false);
	}
}

void AFTSecurityRoomDoor::SetDoorOpen(bool bOpen)
{
	if (bDoorOpen == bOpen)
	{
		return;
	}

	bDoorOpen = bOpen;
	if (bDoorOpen)
	{
		OpenDoorVisual();
	}
	else
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Security room '%s' closing door"), *GetName());
		CloseDoorVisual();
	}
}

void AFTSecurityRoomDoor::SpawnMissingSecurity()
{
	if (!bResponseActive || !SecurityClass || !SpawnPoint || !ReturnPoint)
	{
		return;
	}

	CompactSpawnedSecurityActors();
	const int32 MissingSecurityCount = FMath::Max(SpawnCount - SpawnedSecurityActors.Num(), 0);
	for (int32 SpawnIndex = 0; SpawnIndex < MissingSecurityCount; ++SpawnIndex)
	{
		const int32 SecurityIndex = SpawnedSecurityActors.Num();
		const FVector SpawnLocation = ProjectLocationToNavigation(GetSecuritySlotLocation(SpawnPoint, SecurityIndex));
		const FRotator SpawnRotation = SpawnPoint->GetComponentRotation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AFTSecurityCharacter* SecurityCharacter = GetWorld()->SpawnActor<AFTSecurityCharacter>(
			SecurityClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters
		);
		if (!SecurityCharacter)
		{
			UE_LOG(LogFTSecurity, Warning, TEXT("Security room '%s' failed to spawn security"), *GetName());
			continue;
		}

		SecurityCharacter->IgnorePawnCollisionForDuration(SpawnCollisionIgnoreDuration);
		SpawnedSecurityActors.Add(SecurityCharacter);
		FTimerDelegate DeploymentDelegate;
		DeploymentDelegate.BindUObject(this, &ThisClass::BroadcastDeployment, SecurityCharacter);
		GetWorldTimerManager().SetTimerForNextTick(DeploymentDelegate);
	}
}

void AFTSecurityRoomDoor::BroadcastDeployment(AFTSecurityCharacter* SecurityCharacter)
{
	if (!IsValid(SecurityCharacter) || !PendingTargetActor || !ReturnPoint)
	{
		return;
	}

	FFTSecurityResponsePayloadStruct Payload;
	Payload.SecurityActor = SecurityCharacter;
	Payload.TargetActor = PendingTargetActor;
	Payload.SecurityRoomActor = this;
	Payload.ReportLocation = PendingReportLocation;
	const int32 SecurityIndex = SpawnedSecurityActors.IndexOfByPredicate(
		[SecurityCharacter](const TWeakObjectPtr<AFTSecurityCharacter>& SpawnedSecurity)
		{
			return SpawnedSecurity.Get() == SecurityCharacter;
		}
	);
	Payload.ReturnLocation = ProjectLocationToNavigation(GetSecuritySlotLocation(ReturnPoint, FMath::Max(SecurityIndex, 0)));

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityDeployed, Payload);
	UE_LOG(LogFTSecurity, Log, TEXT("Security room '%s' deployed %s"), *GetName(), *GetNameSafe(SecurityCharacter));
}

void AFTSecurityRoomDoor::CompactSpawnedSecurityActors()
{
	for (int32 SecurityIndex = SpawnedSecurityActors.Num() - 1; SecurityIndex >= 0; --SecurityIndex)
	{
		if (!SpawnedSecurityActors[SecurityIndex].IsValid())
		{
			SpawnedSecurityActors.RemoveAtSwap(SecurityIndex);
		}
	}
}

FVector AFTSecurityRoomDoor::GetSecuritySlotLocation(const USceneComponent* PointComponent, int32 SecurityIndex) const
{
	if (!PointComponent)
	{
		return GetActorLocation();
	}

	const float CenteredSecurityIndex = static_cast<float>(SecurityIndex)
		- static_cast<float>(FMath::Max(SpawnCount, 1) - 1) * 0.5f;
	return PointComponent->GetComponentLocation()
		+ PointComponent->GetRightVector() * SpawnSpacing * CenteredSecurityIndex;
}

FVector AFTSecurityRoomDoor::ProjectLocationToNavigation(const FVector& DesiredLocation) const
{
	const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavigationSystem)
	{
		return DesiredLocation;
	}

	FNavLocation ProjectedLocation;
	if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, NavigationProjectionExtent))
	{
		return ProjectedLocation.Location;
	}

	UE_LOG(
		LogFTSecurity,
		Warning,
		TEXT("Security room '%s' could not project location %s to NavMesh"),
		*GetName(),
		*DesiredLocation.ToString()
	);
	return DesiredLocation;
}
