// Fill out your copyright notice in the Description page of Project Settings.

#include "FTEscapeZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float EscapeCountdownTickInterval = 0.1f;
}

AFTEscapeZoneActor::AFTEscapeZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EscapeCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("EscapeCollision"));
	EscapeCollision->SetupAttachment(SceneRoot);
	EscapeCollision->SetBoxExtent(FVector(200.0f, 200.0f, 120.0f));
	EscapeCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EscapeCollision->SetCollisionObjectType(ECC_WorldDynamic);
	EscapeCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	EscapeCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EscapeCollision->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		EscapeZoneVisualMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisualMaterialFinder(TEXT("/Game/Materials/M_EscapeZoneVisual.M_EscapeZoneVisual"));
	if (VisualMaterialFinder.Succeeded())
	{
		EscapeZoneVisualMaterial = VisualMaterialFinder.Object;
	}
}

void AFTEscapeZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AFTEscapeZoneActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureEscapeZoneVisual();
	RefreshEscapeZoneVisual();

	if (EscapeCollision)
	{
		EscapeCollision->OnComponentBeginOverlap.AddDynamic(this, &AFTEscapeZoneActor::HandleEscapeZoneBeginOverlap);
		EscapeCollision->OnComponentEndOverlap.AddDynamic(this, &AFTEscapeZoneActor::HandleEscapeZoneEndOverlap);
	}
}

void AFTEscapeZoneActor::EnsureEscapeZoneVisual()
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || EscapeZoneVisual)
	{
		return;
	}

	EscapeZoneVisual = NewObject<UStaticMeshComponent>(this, TEXT("EscapeZoneVisual"));
	if (!EscapeZoneVisual)
	{
		return;
	}

	EscapeZoneVisual->SetupAttachment(SceneRoot);
	EscapeZoneVisual->SetStaticMesh(EscapeZoneVisualMesh);
	EscapeZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EscapeZoneVisual->SetCollisionResponseToAllChannels(ECR_Ignore);
	EscapeZoneVisual->SetGenerateOverlapEvents(false);
	EscapeZoneVisual->SetCanEverAffectNavigation(false);
	EscapeZoneVisual->SetCastShadow(false);
	EscapeZoneVisual->bHiddenInGame = false;
	EscapeZoneVisual->RegisterComponent();
}

void AFTEscapeZoneActor::RefreshEscapeZoneVisual()
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	if (!EscapeZoneVisual)
	{
		return;
	}

	if (EscapeCollision)
	{
		const FVector BoxExtent = EscapeCollision->GetUnscaledBoxExtent();
		EscapeZoneVisual->SetRelativeTransform(EscapeCollision->GetRelativeTransform());
		EscapeZoneVisual->SetRelativeScale3D(BoxExtent / 50.0f);
	}

	if (EscapeZoneVisualMaterial)
	{
		EscapeZoneVisual->SetMaterial(0, EscapeZoneVisualMaterial);
	}
}

void AFTEscapeZoneActor::HandleEscapeZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || CurrentEscapeActor)
	{
		return;
	}

	CurrentEscapeActor = OtherActor;
	StartEscapeCountdown(OtherActor);
	OnEscapeZoneEntered(OtherActor);
}

void AFTEscapeZoneActor::HandleEscapeZoneEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor != CurrentEscapeActor)
	{
		return;
	}

	CurrentEscapeActor = nullptr;
	CancelEscapeCountdown(OtherActor);
	OnEscapeZoneExited(OtherActor);
}

void AFTEscapeZoneActor::StartEscapeCountdown(AActor* EscapingActor)
{
	RemainingEscapeTime = EscapeDuration;

	BroadcastEscapeFlowRequest(TAG_FT_Request_Flow_StartEscape, EscapingActor);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->ShowCountdownEscape();
			UIManager->SetCountdownEscapeRemainingTime(RemainingEscapeTime);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EscapeCountdownTimerHandle);
		World->GetTimerManager().SetTimer(
			EscapeCountdownTimerHandle,
			this,
			&AFTEscapeZoneActor::TickEscapeCountdown,
			EscapeCountdownTickInterval,
			true);
	}
}

void AFTEscapeZoneActor::CancelEscapeCountdown(AActor* EscapingActor)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EscapeCountdownTimerHandle);
	}

	RemainingEscapeTime = 0.0f;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HideCountdownEscape();
		}
	}

	BroadcastEscapeFlowRequest(TAG_FT_Request_Flow_CancelEscape, EscapingActor);
}

void AFTEscapeZoneActor::CompleteEscapeCountdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EscapeCountdownTimerHandle);
	}

	AActor* EscapingActor = CurrentEscapeActor.Get();
	CurrentEscapeActor = nullptr;
	RemainingEscapeTime = 0.0f;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->SetCountdownEscapeRemainingTime(0.0f);
			UIManager->HideCountdownEscape();
		}
	}

	BroadcastEscapeFlowRequest(TAG_FT_Request_Flow_CompleteEscape, EscapingActor);
}

void AFTEscapeZoneActor::TickEscapeCountdown()
{
	RemainingEscapeTime = FMath::Max(0.0f, RemainingEscapeTime - EscapeCountdownTickInterval);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->SetCountdownEscapeRemainingTime(RemainingEscapeTime);
		}
	}

	if (RemainingEscapeTime <= 0.0f)
	{
		CompleteEscapeCountdown();
	}
}

void AFTEscapeZoneActor::BroadcastEscapeFlowRequest(const FGameplayTag& RequestTag, AActor* InstigatorActor) const
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = InstigatorActor;
	Payload.TargetActor = const_cast<AFTEscapeZoneActor*>(this);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(RequestTag, Payload);
}
