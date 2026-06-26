

#include "FTProjectileActor.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTProjectileDataAsset.h"


// Sets default values
AFTProjectileActor::AFTProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ProjectileComponent = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileComponent"));

	ProjectileComponent->InitialSpeed = 2500.f;
	ProjectileComponent->MaxSpeed = 4000.f;
	ProjectileComponent->ProjectileGravityScale = 1.0f;
	ProjectileComponent->bRotationFollowsVelocity = true;
	ProjectileComponent->bShouldBounce = true;
	ProjectileComponent->SetAutoActivate(false);

	bIsFlying = false;
}

// Called when the game starts or when spawned
void AFTProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFTProjectileActor::InitProjectile(UFTItemDataAsset* InItemData, const FFTProjectileActionStruct& InProjectileData)
{
}
