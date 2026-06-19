#include "FTAttackComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ProjectFT/Components/FTEquipmentComponent.h"
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
	if (UFTEquipmentComponent* Equipment = GetOwner()->FindComponentByClass<UFTEquipmentComponent>())
	{
		UE_LOG(LogTemp, Log, TEXT("%s received primary attack input."), *GetNameSafe(GetOwner()));
		Equipment->AttackPrimary();
	}
}
