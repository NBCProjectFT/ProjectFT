#include "FTBubbleStackTrapActor.h"

#include "Components/BoxComponent.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_BubbleStack.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTUseDataEffectLibrary.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Player/FTPlayerCharacter.h"

AFTBubbleStackTrapActor::AFTBubbleStackTrapActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrapCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("TrapCollision"));
	TrapCollision->SetupAttachment(SceneRoot);
	TrapCollision->SetBoxExtent(FVector(100.0f, 100.0f, 80.0f));
	TrapCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TrapCollision->SetCollisionObjectType(ECC_WorldDynamic);
	TrapCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	TrapCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TrapCollision->SetGenerateOverlapEvents(true);

	BubbleStackUseData.UseEffects.Add(UFTGE_BubbleStack::StaticClass());
	BubbleStackUseData.EffectMagnitudes.Add(TAG_FT_Data_Duration, 3.0f);
	BubbleStackUseData.EffectMagnitudes.Add(TAG_FT_Data_BubbleDuration, 5.0f);
}

void AFTBubbleStackTrapActor::BeginPlay()
{
	Super::BeginPlay();

	if (TrapCollision)
	{
		TrapCollision->OnComponentBeginOverlap.AddDynamic(this, &AFTBubbleStackTrapActor::HandleTrapBeginOverlap);
	}
}

void AFTBubbleStackTrapActor::HandleTrapBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!ShouldAffectActor(OtherActor))
	{
		return;
	}

	if (bTriggerOncePerActor && TriggeredActors.Contains(OtherActor))
	{
		return;
	}

	const int32 AppliedCount = ApplyBubbleStacksToActor(OtherActor);
	if (AppliedCount > 0)
	{
		TriggeredActors.Add(OtherActor);
		UE_LOG(LogFTPlayer, Log, TEXT("BubbleStackTrap applied %d stack effect(s) to %s."),
			AppliedCount,
			*GetNameSafe(OtherActor));
	}
}

int32 AFTBubbleStackTrapActor::ApplyBubbleStacksToActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || StackCount <= 0)
	{
		return 0;
	}

	int32 AppliedCount = 0;
	for (int32 StackIndex = 0; StackIndex < StackCount; ++StackIndex)
	{
		AppliedCount += UFTUseDataEffectLibrary::ApplyUseEffectsToActor(
			this,
			TargetActor,
			BubbleStackUseData);
	}

	return AppliedCount;
}

bool AFTBubbleStackTrapActor::ShouldAffectActor(AActor* OtherActor) const
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return false;
	}

	if (bOnlyAffectPlayer && !Cast<AFTPlayerCharacter>(OtherActor))
	{
		return false;
	}

	return true;
}
