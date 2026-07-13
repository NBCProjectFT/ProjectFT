#include "FTShoppingPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"

AFTShoppingPoint::AFTShoppingPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 에디터에서 쇼핑 영역을 박스로 확인하기 위한 컴포넌트다.
	ShoppingAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ShoppingAreaBox"));
	ShoppingAreaBox->SetupAttachment(SceneRoot);
	ShoppingAreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoppingAreaBox->SetBoxExtent(BoxExtent);

	// 에디터에서 NPC가 도착 후 바라볼 방향을 확인하기 위한 화살표다.
	LookDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LookDirectionArrow"));
	LookDirectionArrow->SetupAttachment(SceneRoot);
	LookDirectionArrow->SetArrowSize(1.5f);
	LookDirectionArrow->SetArrowColor(FColor::Cyan);

	Tags.AddUnique(TEXT("CustomerShoppingPoint"));
}

bool AFTShoppingPoint::CanSelectPreferred() const
{
	return CurrentSelectors < MaxSelectors && SelectionWeight > 0.0f;
}

void AFTShoppingPoint::Reserve()
{
	++CurrentSelectors;
}

void AFTShoppingPoint::Release()
{
	CurrentSelectors = FMath::Max(CurrentSelectors - 1, 0);
}

bool AFTShoppingPoint::GetRandomShoppingLocation(UObject* WorldContextObject, FVector& OutLocation) const
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : GetWorld();
	const UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	if (!NavigationSystem)
	{
		OutLocation = GetActorLocation();
		return false;
	}

	const FTransform ActorTransform = GetActorTransform();
	const FVector ProjectionExtent(BoxExtent.X, BoxExtent.Y, FMath::Max(BoxExtent.Z, 100.0f));

	for (int32 AttemptIndex = 0; AttemptIndex < 10; ++AttemptIndex)
	{
		// Move To 도착 반경 때문에 박스 밖에서 멈추지 않도록 가장자리보다 안쪽에서 후보 위치를 뽑는다.
		const FVector LocalOffset(
			FMath::RandRange(-BoxExtent.X, BoxExtent.X),
			FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
			0.0f);
		const FVector CandidateLocation = ActorTransform.TransformPosition(LocalOffset);

		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(CandidateLocation, ProjectedLocation, ProjectionExtent))
		{
			OutLocation = ProjectedLocation.Location;
			return true;
		}
	}

	FNavLocation ProjectedCenterLocation;
	if (NavigationSystem->ProjectPointToNavigation(GetActorLocation(), ProjectedCenterLocation, ProjectionExtent))
	{
		OutLocation = ProjectedCenterLocation.Location;
		return true;
	}

	OutLocation = GetActorLocation();
	return false;
}

/*
FVector AFTShoppingPoint::GetShoppingLookLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 500.0f;
}
*/

void AFTShoppingPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ShoppingAreaBox)
	{
		ShoppingAreaBox->SetBoxExtent(BoxExtent);
	}
}

void AFTShoppingPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bDrawDebugArea)
	{
		return;
	}

	DrawDebugBox(
		GetWorld(),
		GetActorLocation(),
		BoxExtent,
		GetActorQuat(),
		FColor::Cyan,
		false,
		0.0f,
		0,
		2.0f);
}
