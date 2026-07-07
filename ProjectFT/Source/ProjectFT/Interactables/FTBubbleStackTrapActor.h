#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTBubbleStackTrapActor.generated.h"

class UBoxComponent;
class USceneComponent;

/**
 * Overlap trap that applies BubbleStack effects to the player.
 *
 * The default setup applies UFTGE_BubbleStack five times, so the target's
 * existing UFTGA_BubbleStackTrap turns the stacks into UFTGE_BubbleTrap.
 */
UCLASS()
class PROJECTFT_API AFTBubbleStackTrapActor : public AActor
{
	GENERATED_BODY()

public:
	AFTBubbleStackTrapActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTrapBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "FT|Trap")
	int32 ApplyBubbleStacksToActor(AActor* TargetActor);

	bool ShouldAffectActor(AActor* OtherActor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Trap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Trap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TrapCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Trap", meta = (ClampMin = "1"))
	int32 StackCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Trap")
	FTItemUseStruct BubbleStackUseData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Trap")
	bool bOnlyAffectPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Trap")
	bool bTriggerOncePerActor = true;

private:
	TSet<TWeakObjectPtr<AActor>> TriggeredActors;
};
