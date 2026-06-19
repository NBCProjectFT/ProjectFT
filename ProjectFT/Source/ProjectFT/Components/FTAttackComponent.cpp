#include "FTAttackComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InputCoreTypes.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "TimerManager.h"

UFTAttackComponent::UFTAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	TryBindInput();
}

void UFTAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer);
	if (BoundController && AttackInputComponent)
	{
		BoundController->PopInputComponent(AttackInputComponent);
	}

	AttackInputComponent = nullptr;
	BoundController = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UFTAttackComponent::TryBindInput()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = OwnerPawn
		? Cast<APlayerController>(OwnerPawn->GetController())
		: nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		GetWorld()->GetTimerManager().SetTimer(
			BindRetryTimer, this, &UFTAttackComponent::TryBindInput, 0.1f, false);
		return;
	}

	BoundController = PlayerController;
	AttackInputComponent = NewObject<UInputComponent>(GetOwner(), TEXT("WeaponAttackInput"));
	AttackInputComponent->RegisterComponent();
	AttackInputComponent->BindKey(
		EKeys::LeftMouseButton, IE_Pressed, this, &UFTAttackComponent::HandleAttackPressed);
	BoundController->PushInputComponent(AttackInputComponent);
}

void UFTAttackComponent::HandleAttackPressed()
{
	AActor* Requester = GetOwner();
	if (!Requester)
	{
		return;
	}

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = Requester;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		TAG_FT_Weapon_Action_Primary, Payload);
}
