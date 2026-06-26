#pragma once

#include "CoreMinimal.h"
#include "FTItemActor.h"
#include "ProjectFT/Struct/FTProjectileActionStruct.h"
#include "FTProjectileActor.generated.h"

class UProjectileMovementComponent;
class UFTItemDataAsset;

UCLASS()
class PROJECTFT_API AFTProjectileActor : public AFTItemActor
{
	GENERATED_BODY()

public:
	AFTProjectileActor();

protected:
	virtual void BeginPlay() override;

public:
	void InitProjectile(
		UFTItemDataAsset* InItemData,
		const FFTProjectileActionStruct& InProjectileData
	);

	// void LaunchProjectile(const FVector& Direction);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Projectile")
	FFTProjectileActionStruct ProjectileActionData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Projectile")
	bool bIsFlying = false;
};