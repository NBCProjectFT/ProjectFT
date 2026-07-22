#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "FTSecurityRoomDoor.generated.h"

class AFTSecurityCharacter;
class USceneComponent;
class UStaticMeshComponent;
struct FFTNPCReportPayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;
struct FFTSecurityResponsePayloadStruct;

UCLASS()
class PROJECTFT_API AFTSecurityRoomDoor : public AActor
{
	GENERATED_BODY()

public:
	AFTSecurityRoomDoor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom")
	TObjectPtr<USceneComponent> SpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom")
	TObjectPtr<USceneComponent> ReturnPoint;

	/** 일반 보안요원(잡는용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn")
	TSubclassOf<AFTSecurityCharacter> SecurityClass;
	
	/** 공격 보안요원(공격용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn")
	TSubclassOf<AFTSecurityCharacter> AttackSecurityClass;

	/** 소환할 일반 보안요원의 수이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn", meta = (ClampMin = "0"))
	int32 SpawnCount = 1;
	
	/** 소환할 공격 보안요원의 수이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn", meta = (ClampMin = "0"))
	int32 AttackSecuritySpawnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn", meta = (ClampMin = "0.0"))
	float SpawnDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn", meta = (ClampMin = "0.0"))
	float SpawnSpacing = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Spawn", meta = (ClampMin = "0.0"))
	float SpawnCollisionIgnoreDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|SecurityRoom|Navigation")
	FVector NavigationProjectionExtent = FVector(200.0f, 200.0f, 500.0f);

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|SecurityRoom")
	void OpenDoorVisual();

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|SecurityRoom")
	void CloseDoorVisual();

private:
	FGameplayMessageListenerHandle SecurityCalledListenerHandle;
	FGameplayMessageListenerHandle SecurityTargetCapturedListenerHandle;
	FGameplayMessageListenerHandle ChaseEndedListenerHandle;
	FGameplayMessageListenerHandle SecurityReturnedListenerHandle;
	FTimerHandle SpawnTimerHandle;

	UPROPERTY()
	TObjectPtr<AActor> PendingTargetActor;

	FVector PendingReportLocation = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AFTSecurityCharacter>> SpawnedSecurityActors;
	bool bResponseActive = false;
	bool bDoorOpen = false;

	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnSecurityTargetCaptured(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnSecurityReturned(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload);
	void SetDoorOpen(bool bOpen);
	void SpawnMissingSecurity();
	void BroadcastDeployment(AFTSecurityCharacter* SecurityCharacter);
	void CompactSpawnedSecurityActors();
	void ReadyDespawn(AActor* SecurityActor);
	FVector GetSecuritySlotLocation(const USceneComponent* PointComponent, int32 SecurityIndex) const;
	FVector ProjectLocationToNavigation(const FVector& DesiredLocation) const;
};
