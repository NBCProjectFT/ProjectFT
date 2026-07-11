

#include "FTCaptureDestination.h"

#include "Components/SceneComponent.h"

AFTCaptureDestination::AFTCaptureDestination()
{
	PrimaryActorTick.bCanEverTick = false;

	// 위치만 필요한 마커라 씬 컴포넌트 루트만 둔다.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
