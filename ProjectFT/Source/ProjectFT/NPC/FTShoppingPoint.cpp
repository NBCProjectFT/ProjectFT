#include "FTShoppingPoint.h"

#include "Components/SceneComponent.h"

AFTShoppingPoint::AFTShoppingPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Tags.AddUnique(TEXT("CustomerShoppingPoint"));
}
