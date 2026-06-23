#include "FTProjectileActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

namespace
{
UAbilitySystemComponent* ResolveAbilitySystemComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (UAbilitySystemComponent* AbilitySystemComponent =
		Actor->FindComponentByClass<UAbilitySystemComponent>())
	{
		return AbilitySystemComponent;
	}
	if (const IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(Actor))
	{
		return AbilitySystemInterface->GetAbilitySystemComponent();
	}
	return nullptr;
}
}

AFTProjectileActor::AFTProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
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
	UFTItemDataAsset* InItemData,
	FVector InLaunchDirection,
	float InSpeed,
	float InLifeSpan,
	float InGravityScale,
	float InCollisionRadius)
{
	Damage = FMath::Max(0.0f, InDamage);
	SourceAbilitySystem = InSourceAbilitySystem;
	EffectClass = InEffectClass;
	InitializeFromItemData(InItemData);

	LaunchDirection = InLaunchDirection.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = GetActorForwardVector();
	}
	SetActorRotation(LaunchDirection.Rotation());

	InitialLifeSpan = FMath::Max(0.1f, InLifeSpan);
	SetLifeSpan(InitialLifeSpan);

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
		ProjectileMovement->Velocity = LaunchDirection * Speed;
	}
}

void AFTProjectileActor::InitializeFromItemData(UFTItemDataAsset* InItemData)
{
	ItemData = InItemData;
	UE_LOG(LogFTItem, Verbose, TEXT("Projectile '%s' received ItemData: %s"),
		*GetName(), *GetNameSafe(ItemData.Get()));
	UpdateAppearance();
}

void AFTProjectileActor::UpdateAppearance()
{
	if (!ItemData)
	{
		UE_LOG(LogFTItem, Warning,
			TEXT("Projectile '%s' cannot update appearance: ItemData is null."),
			*GetName());
		return;
	}
	if (ItemData->ItemData.ItemMesh.IsNull())
	{
		UE_LOG(LogFTItem, Warning,
			TEXT("Projectile '%s' cannot update appearance: ItemData '%s' has no ItemMesh."),
			*GetName(), *GetNameSafe(ItemData.Get()));
		return;
	}

	UStaticMesh* LoadedMesh = ItemData->ItemData.ItemMesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		UE_LOG(LogFTItem, Warning,
			TEXT("Projectile '%s' failed to load ItemMesh from ItemData '%s'."),
			*GetName(), *GetNameSafe(ItemData.Get()));
		return;
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	if (StaticMeshComponents.IsEmpty())
	{
		UE_LOG(LogFTItem, Warning,
			TEXT("Projectile '%s' has no StaticMeshComponent to apply ItemMesh '%s'."),
			*GetName(), *GetNameSafe(LoadedMesh));
		return;
	}

	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (StaticMeshComponent)
		{
			StaticMeshComponent->SetStaticMesh(LoadedMesh);
			StaticMeshComponent->SetRelativeScale3D(ItemData->ItemMeshScale);
			StaticMeshComponent->EmptyOverrideMaterials();
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticMeshComponent->SetVisibility(true, true);
			StaticMeshComponent->SetHiddenInGame(false, true);
			StaticMeshComponent->SetVisibleInSceneCaptureOnly(false);
			StaticMeshComponent->SetCastHiddenShadow(false);
			StaticMeshComponent->MarkRenderStateDirty();
		}
	}

	UE_LOG(LogFTItem, Verbose,
		TEXT("Projectile '%s' applied ItemMesh '%s' from ItemData '%s' to %d StaticMeshComponent(s)."),
		*GetName(), *GetNameSafe(LoadedMesh), *GetNameSafe(ItemData.Get()),
		StaticMeshComponents.Num());
}

void AFTProjectileActor::SpawnItemOnImpact(const FHitResult& Hit)
{
	if (!ItemData || !ItemData->bSpawnItemOnProjectileImpact || !GetWorld())
	{
		return;
	}

	const FVector SpawnLocation = Hit.ImpactPoint.IsNearlyZero()
		? GetActorLocation()
		: Hit.ImpactPoint + Hit.ImpactNormal * 4.0f;
	const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFTItemActor* SpawnedItem = GetWorld()->SpawnActor<AFTItemActor>(
		AFTItemActor::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParams);
	if (!SpawnedItem)
	{
		return;
	}

	SpawnedItem->InitializeFromItemData(ItemData);
	SpawnedItem->SetEquipped(false);
}

void AFTProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	UpdateAppearance();
	CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	CollisionComponent->OnComponentHit.AddUniqueDynamic(
		this, &AFTProjectileActor::HandleProjectileHit);

	if (ProjectileMovement)
	{
		const float Speed = FMath::Max(1.0f, ProjectileMovement->InitialSpeed);
		ProjectileMovement->Velocity = LaunchDirection * Speed;
	}
}

void AFTProjectileActor::HandleProjectileHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bHasImpacted ||
		!OtherActor ||
		OtherActor == this ||
		OtherActor == GetOwner() ||
		OtherActor == GetInstigator())
	{
		return;
	}
	bHasImpacted = true;

	UAbilitySystemComponent* TargetASC = ResolveAbilitySystemComponent(OtherActor);
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
	SpawnItemOnImpact(Hit);
	Destroy();
}
