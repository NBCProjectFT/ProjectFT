#include "FTProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Sound/SoundBase.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "ProjectFT/Struct/FTProjectileActorStruct.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "Kismet/GameplayStatics.h"

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
		ProjectileMovementComponent->SetUpdatedComponent(nullptr);
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
		ProjectileMovementComponent->SetUpdatedComponent(ProjectileCollisionComponent);
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
		bool bShouldPostponeExplosion = false;
		if (ProjectileActorData)
		{
			const FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
			if (ProjectileActorData->ItemData.UseData.EffectMagnitudes.Contains(DamageTag))
			{
				const float DamageValue = ProjectileActorData->ItemData.UseData.EffectMagnitudes[DamageTag];
				if (DamageValue < 0.0f && (!HitActor || !HitActor->Implements<UFTDamageable>()))
				{
					bShouldPostponeExplosion = true;
				}
			}
		}

		if (!bShouldPostponeExplosion)
		{
			Explode(HitActor);
			return;
		}
	}

	// 폭발한 경우엔 위에서 return하므로 여기 오지 않는다 — 착탄음과 폭발음이 겹치지 않는다.
	PlayImpactSound();

	if (bDestroyOnImpact)
	{
		Destroy();
	}
	
	// 날아가던 원래 속도를 미리 보존합니다.
	FVector SavedVelocity = FVector::ZeroVector;
	if (ProjectileMovementComponent)
	{
		SavedVelocity = ProjectileMovementComponent->Velocity;
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileCollisionComponent->SetSimulatePhysics(false);
	}

	if (MeshComponent)
	{
		// 루트 컴포넌트를 메쉬로 변경하고 분리
		SetRootComponent(MeshComponent);
		MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// 부모(AFTItemActor)의 물리/콜리전 설정 재사용
		SetupPhysicsAndCollision();

		// 보존했던 속도를 물리 메쉬에 전달하여 관성을 유지시킵니다.
		MeshComponent->SetPhysicsLinearVelocity(SavedVelocity);

		// 땅에 부딪혔을 때 실감 나게 구르도록 랜덤한 회전력 추가
		FVector RandomAngular = FVector(
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f)
		);
		MeshComponent->SetPhysicsAngularVelocityInDegrees(RandomAngular);
	}
}

void AFTProjectileActor::PlayImpactSound()
{
	if (!ProjectileActorData)
	{
		return;
	}

	const FFTProjectileActorStruct& ProjectileData = ProjectileActorData->ProjectileActorData;
	if (!ProjectileData.ImpactSound)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Overlap 모드에서 한 프레임에 여러 대상과 겹치면 같은 소리가 뭉쳐 터지므로 간격으로 거른다.
	const float Now = World->GetTimeSeconds();
	if (ProjectileData.ImpactSoundMinInterval > 0.0f
		&& (Now - LastImpactSoundTime) < ProjectileData.ImpactSoundMinInterval)
	{
		return;
	}
	LastImpactSoundTime = Now;

	// 충돌 지점(투사체 현재 위치)에서 재생하고 잊는다 — 짧은 소리라 액터에 붙일 필요가 없고,
	// 착탄 직후 이 액터가 파괴되거나 물리 메시로 전환돼도 소리가 끊기지 않는다.
	UGameplayStatics::PlaySoundAtLocation(this, ProjectileData.ImpactSound, GetActorLocation());
}

void AFTProjectileActor::Explode(AActor* DirectHitActor)
{
	if (!ProjectileActorData || bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	ReceiveExplode(GetActorLocation());

	// 폭발음은 bHasExploded 가드 덕에 저절로 1회다. 액터가 곧 파괴될 수 있으므로 붙이지 않고 위치에서 재생한다.
	if (USoundBase* SoundToPlay = ProjectileActorData->ProjectileActorData.ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
	}

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

			if (TargetActor && TargetActor->Implements<UFTDamageable>())
			{
				float DamageValue = 0.0f;
				if (ProjectileActorData)
				{
					const FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
					if (ProjectileActorData->ItemData.UseData.EffectMagnitudes.Contains(DamageTag))
					{
						DamageValue = ProjectileActorData->ItemData.UseData.EffectMagnitudes[DamageTag];
					}
				}

				if (DamageValue < 0.0f)
				{
					UGameplayStatics::ApplyDamage(
						TargetActor,
						FMath::Abs(DamageValue),
						GetInstigatorController(),
						this,
						UDamageType::StaticClass()
					);
				}
			}
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

	// 매대 등 파괴 가능한 타겟(IFTDamageable)은 에셋에 Data.Damage가 0보다 크게 설정된 경우에만 즉시 충돌/폭발 대상으로 인정합니다.
	if (TargetActor->Implements<UFTDamageable>())
	{
		// 단, ASC를 가진 GAS 액터(캐릭터/AI)는 데미지가 없어도 GameplayEffect(버블/스턴 등 비살상 효과)를 받을 수 있으므로 항상 유효 타겟으로 본다.
		// 캐릭터도 IFTDamageable을 구현하므로, ASC 유무로 '파괴형 프롭(매대)'과 'GE 대상(캐릭터)'을 구분한다.
		// (이 구분이 없으면 Data.Damage가 없는 비살상 투사체가 캐릭터에 대해 무효 처리돼 버블/스턴이 아예 적용되지 않는다.)
		if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			return true;
		}

		const FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
		if (ProjectileActorData->ItemData.UseData.EffectMagnitudes.Contains(DamageTag))
		{
			return ProjectileActorData->ItemData.UseData.EffectMagnitudes[DamageTag] < 0.0f;
		}
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
