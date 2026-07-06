#include "FTProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "ProjectFT/Struct/FTProjectileActorStruct.h"

AFTProjectileActor::AFTProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollisionComponent"));
	ProjectileCollisionComponent->InitSphereRadius(20.0f);
	ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProjectileCollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileCollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	// 카메라 붐(스프링암)은 ECC_Camera 프로브로 벽을 감지한다. 발사체가 이 채널을 Block하면 카메라가 발사체에 걸려 확 당겨지므로
	// 발사체는 카메라 채널을 항상 무시한다(발사 지점이 카메라 근처여도 시야에 영향 없음).
	ProjectileCollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ProjectileCollisionComponent->SetGenerateOverlapEvents(true);
	ProjectileCollisionComponent->SetNotifyRigidBodyCollision(true);

	SetRootComponent(ProjectileCollisionComponent);

	if (MeshComponent)
	{
		MeshComponent->SetupAttachment(ProjectileCollisionComponent);
		MeshComponent->SetSimulatePhysics(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
	}

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->UpdatedComponent = ProjectileCollisionComponent;
		ProjectileMovementComponent->InitialSpeed = 3000.0f;
		ProjectileMovementComponent->MaxSpeed = 3000.0f;
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
		ProjectileMovementComponent->bRotationFollowsVelocity = true;
		ProjectileMovementComponent->bShouldBounce = false;
	}
}

void AFTProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->OnComponentBeginOverlap.AddDynamic(
			this,
			&AFTProjectileActor::OnProjectileBeginOverlap
		);

		ProjectileCollisionComponent->OnComponentHit.AddDynamic(
			this,
			&AFTProjectileActor::OnProjectileHit
		);
	}
}

void AFTProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFTProjectileActor::InitializeProjectile(
	UFTProjectileActorDataAsset* InProjectileActorData,
	const FVector& FireDirection,
	AActor* InOwnerActor)
{
	if (!InProjectileActorData)
	{
		return;
	}

	ProjectileActorData = InProjectileActorData;

	ItemData = InProjectileActorData;
	UpdateAppearance();

	SetOwner(InOwnerActor);
	SetInstigator(Cast<APawn>(InOwnerActor));

	const FFTProjectileActorStruct& ProjectileData =
		InProjectileActorData->ProjectileActorData;

	ConfigureProjectileCollision(ProjectileData);

	if (ProjectileCollisionComponent && InOwnerActor)
	{
		ProjectileCollisionComponent->IgnoreActorWhenMoving(InOwnerActor, true);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->UpdatedComponent = ProjectileCollisionComponent;
		ProjectileMovementComponent->InitialSpeed = ProjectileData.ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = ProjectileData.MaxSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = ProjectileData.GravityScale;
		ProjectileMovementComponent->Velocity =
			FireDirection.GetSafeNormal() * ProjectileData.ProjectileSpeed;
	}

	MinDamageSpeed = ProjectileData.MinDamageSpeed;
	bDamageOnlyOnce = ProjectileData.bDamageOnlyOnce;
	bDestroyOnImpact = ProjectileData.bDestroyOnImpact;

	if (ProjectileData.LifeTime > 0.0f)
	{
		SetLifeSpan(ProjectileData.LifeTime);
	}
}

void AFTProjectileActor::InitializeHeldProjectile(
	UFTProjectileActorDataAsset* InProjectileActorData,
	AActor* InOwnerActor)
{
	if (!InProjectileActorData)
	{
		return;
	}

	ProjectileActorData = InProjectileActorData;

	ItemData = InProjectileActorData;
	UpdateAppearance();

	SetOwner(InOwnerActor);
	SetInstigator(Cast<APawn>(InOwnerActor));

	const FFTProjectileActorStruct& ProjectileData =
		InProjectileActorData->ProjectileActorData;

	MinDamageSpeed = ProjectileData.MinDamageSpeed;
	bDamageOnlyOnce = ProjectileData.bDamageOnlyOnce;
	bDestroyOnImpact = ProjectileData.bDestroyOnImpact;
	HitActors.Reset();
	bHasExploded = false;

	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->SetSphereRadius(ProjectileData.CollisionRadius);
		ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileCollisionComponent->SetGenerateOverlapEvents(false);

		if (InOwnerActor)
		{
			ProjectileCollisionComponent->IgnoreActorWhenMoving(InOwnerActor, true);
		}
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->UpdatedComponent = ProjectileCollisionComponent;
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

	SetLifeSpan(0.0f);
}

void AFTProjectileActor::ReleaseProjectile(const FVector& FireDirection)
{
	if (!ProjectileActorData)
	{
		return;
	}

	const FFTProjectileActorStruct& ProjectileData =
		ProjectileActorData->ProjectileActorData;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HitActors.Reset();
	bHasExploded = false;

	ConfigureProjectileCollision(ProjectileData);

	if (ProjectileCollisionComponent)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			ProjectileCollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
		}

		if (APawn* InstigatorPawn = GetInstigator())
		{
			ProjectileCollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
		}
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->UpdatedComponent = ProjectileCollisionComponent;
		ProjectileMovementComponent->InitialSpeed = ProjectileData.ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = ProjectileData.MaxSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = ProjectileData.GravityScale;
		ProjectileMovementComponent->Velocity =
			FireDirection.GetSafeNormal() * ProjectileData.ProjectileSpeed;
		ProjectileMovementComponent->Activate(true);
	}

	MinDamageSpeed = ProjectileData.MinDamageSpeed;
	bDamageOnlyOnce = ProjectileData.bDamageOnlyOnce;
	bDestroyOnImpact = ProjectileData.bDestroyOnImpact;

	if (ProjectileData.LifeTime > 0.0f)
	{
		SetLifeSpan(ProjectileData.LifeTime);
	}
}

void AFTProjectileActor::ConfigureProjectileCollision(const FFTProjectileActorStruct& ProjectileData)
{
	if (!ProjectileCollisionComponent)
	{
		return;
	}

	ProjectileCollisionComponent->SetSphereRadius(ProjectileData.CollisionRadius);
	ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProjectileCollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileCollisionComponent->SetGenerateOverlapEvents(true);

	if (ProjectileData.bUseOverlapCollision)
	{
		ProjectileCollisionComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
		ProjectileCollisionComponent->SetNotifyRigidBodyCollision(false);
	}
	else
	{
		ProjectileCollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		ProjectileCollisionComponent->SetNotifyRigidBodyCollision(true);
	}

	// SetCollisionResponseToAllChannels가 위에서 카메라 응답까지 덮어썼으므로, 카메라 채널 무시를 다시 적용한다
	// (스프링암 프로브가 발사체에 걸려 카메라가 당겨지는 것 방지).
	ProjectileCollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AFTProjectileActor::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!ProjectileActorData)
	{
		return;
	}

	if (!ProjectileActorData->ProjectileActorData.bUseOverlapCollision)
	{
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	const float CurrentSpeed = GetVelocity().Size();
	if (CurrentSpeed < MinDamageSpeed)
	{
		return;
	}

	if (bDamageOnlyOnce && HitActors.Contains(OtherActor))
	{
		return;
	}

	HitActors.Add(OtherActor);

	HandleProjectileImpact(OtherActor);
}

void AFTProjectileActor::OnProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!ProjectileActorData)
	{
		return;
	}

	if (ProjectileActorData->ProjectileActorData.bUseOverlapCollision)
	{
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	const float CurrentSpeed = GetVelocity().Size();
	if (CurrentSpeed < MinDamageSpeed)
	{
		return;
	}

	if (bDamageOnlyOnce && HitActors.Contains(OtherActor))
	{
		return;
	}

	HitActors.Add(OtherActor);

	HandleProjectileImpact(OtherActor);
}

void AFTProjectileActor::HandleProjectileImpact(AActor* HitActor)
{
	if (!ProjectileActorData)
	{
		return;
	}

	const FFTProjectileActorStruct& ProjectileData =
		ProjectileActorData->ProjectileActorData;

	if (ProjectileData.bApplyEffectsOnImpact && IsValidDirectHitTarget(HitActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Send TargetHit Event: %s"), *GetNameSafe(HitActor));
		SendTargetHitEvent(HitActor);
	}

	if (ProjectileData.bExplodeOnImpact)
	{
		Explode(HitActor);
		return;
	}

	if (bDestroyOnImpact)
	{
		Destroy();
	}
	
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ProjectileCollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		ProjectileCollisionComponent->SetSimulatePhysics(true);
	}
}

void AFTProjectileActor::Explode(AActor* DirectHitActor)
{
	if (!ProjectileActorData || bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FFTProjectileActorStruct& ProjectileData =
		ProjectileActorData->ProjectileActorData;

	TArray<FOverlapResult> OverlapResults;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (ProjectileData.bIgnoreOwnerInExplosion)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			QueryParams.AddIgnoredActor(OwnerActor);
		}

		if (APawn* InstigatorPawn = GetInstigator())
		{
			QueryParams.AddIgnoredActor(InstigatorPawn);
		}
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(
		ProjectileData.ExplosionTargetObjectChannel.GetValue()
	);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ProjectileData.ExplosionRadius),
		QueryParams
	);

	TSet<AActor*> ExplosionTargets;

	if (DirectHitActor)
	{
		ExplosionTargets.Add(DirectHitActor);
	}

	if (bHasOverlap)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* TargetActor = Result.GetActor();

			if (!TargetActor || TargetActor == this)
			{
				continue;
			}

			if (ProjectileData.bIgnoreOwnerInExplosion &&
				(TargetActor == GetOwner() || TargetActor == GetInstigator()))
			{
				continue;
			}

			ExplosionTargets.Add(TargetActor);
		}
	}

	if (ProjectileData.bApplyEffectsToExplosionTargets)
	{
		for (AActor* TargetActor : ExplosionTargets)
		{
			SendTargetHitEvent(TargetActor);
		}
	}

	if (ProjectileData.bDestroyAfterExplosion)
	{
		Destroy();
	}
}

void AFTProjectileActor::SendTargetHitEvent(AActor* TargetActor)
{
	if (!TargetActor || !ProjectileActorData)
	{
		return;
	}

	AActor* SourceActor = GetOwner();

	if (!SourceActor)
	{
		SourceActor = GetInstigator();
	}

	if (!SourceActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);

	FGameplayEffectContextHandle ContextHandle;

	if (SourceASC)
	{
		ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(SourceActor, this);
		ContextHandle.AddSourceObject(ProjectileActorData);
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_FT_Event_TargetHit;
	EventData.Instigator = SourceActor;
	EventData.Target = TargetActor;
	EventData.OptionalObject = ProjectileActorData;
	EventData.OptionalObject2 = this;
	EventData.ContextHandle = ContextHandle;

	if (!SourceASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug] TargetHit event skipped: SourceASC is null. Source=%s Target=%s ProjectileData=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(TargetActor),
			*GetNameSafe(ProjectileActorData));
		return;
	}

	const int32 ActivatedAbilityCount = SourceASC->HandleGameplayEvent(TAG_FT_Event_TargetHit, &EventData);
	UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug] TargetHit event sent. ActivatedAbilities=%d Source=%s Target=%s ProjectileData=%s"),
		ActivatedAbilityCount,
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		*GetNameSafe(ProjectileActorData));
}

bool AFTProjectileActor::IsValidDirectHitTarget(AActor* TargetActor) const
{
	if (!TargetActor || !ProjectileActorData)
	{
		return false;
	}

	const FFTProjectileActorStruct& ProjectileData =
		ProjectileActorData->ProjectileActorData;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		if (PrimitiveComponent->GetCollisionObjectType() ==
			ProjectileData.DirectHitTargetObjectChannel.GetValue())
		{
			return true;
		}
	}

	return false;
}

void AFTProjectileActor::LifeSpanExpired()
{
	if (ProjectileActorData &&
		ProjectileActorData->ProjectileActorData.bExplodeOnLifeEnd)
	{
		Explode(nullptr);

		if (!IsPendingKillPending())
		{
			Destroy();
		}

		return;
	}

	Super::LifeSpanExpired();
}
