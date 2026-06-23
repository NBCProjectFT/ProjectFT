#include "FTProjectileActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"

AFTProjectileActor::AFTProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 5.0f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(8.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AFTProjectileActor::InitializeProjectile(float InDamage,
	UAbilitySystemComponent* InSourceAbilitySystem,
	TSubclassOf<UGameplayEffect> InEffectClass,
	float InSpeed,
	float InLifeSpan,
	float InGravityScale,
	float InCollisionRadius)
{
	Damage = FMath::Max(0.0f, InDamage);
	SourceAbilitySystem = InSourceAbilitySystem;
	EffectClass = InEffectClass;
	InitialLifeSpan = FMath::Max(0.1f, InLifeSpan);

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(FMath::Max(1.0f, InCollisionRadius), true);
	}
	if (ProjectileMovement)
	{
		const float Speed = FMath::Max(1.0f, InSpeed);
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->ProjectileGravityScale = FMath::Max(0.0f, InGravityScale);
	}
}

void AFTProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->OnComponentHit.AddUniqueDynamic(
		this, &AFTProjectileActor::HandleProjectileHit);
	CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
}

void AFTProjectileActor::HandleProjectileHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = OtherActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		if (const IAbilitySystemInterface* AbilitySystemInterface =
			Cast<IAbilitySystemInterface>(OtherActor))
		{
			TargetASC = AbilitySystemInterface->GetAbilitySystemComponent();
		}
	}
	if (SourceAbilitySystem && TargetASC)
	{
		TSubclassOf<UGameplayEffect> AppliedEffectClass = EffectClass;
		if (!AppliedEffectClass)
		{
			AppliedEffectClass = UFTGE_Damage::StaticClass();
		}
		FGameplayEffectContextHandle Context = SourceAbilitySystem->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle Spec = SourceAbilitySystem->MakeOutgoingSpec(
			AppliedEffectClass, 1.0f, Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -Damage);
			SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}
	else
	{
		AController* InstigatorController = GetInstigator()
			? GetInstigator()->GetController()
			: nullptr;
		UGameplayStatics::ApplyDamage(
			OtherActor, Damage, InstigatorController, this, nullptr);
	}
	Destroy();
}
