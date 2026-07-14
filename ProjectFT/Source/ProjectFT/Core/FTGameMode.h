// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTGameMode.generated.h"

class UFTGameFlowSubsystem;
class UStaticMesh;

/**
 * 
 */
UCLASS()
class PROJECTFT_API AFTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFTGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidStart();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidFail();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void HandleRaidEscape();

private:
	void GrantRaidStartTestItemsIfNeeded(const UFTGameFlowSubsystem* GameFlowSubsystem) const;
	void SpawnRuntimeMeshTest();
	TArray<TSoftObjectPtr<UStaticMesh>> CollectRuntimeSpawnMeshes() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Inventory Test", meta = (AllowPrivateAccess = "true"))
	bool bGrantRaidStartTestItems = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Inventory Test", meta = (AllowPrivateAccess = "true", EditCondition = "bGrantRaidStartTestItems"))
	TArray<FTStorageItemStruct> RaidStartTestItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true"))
	bool bEnableRuntimeMeshSpawnTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true"))
	TArray<TSoftObjectPtr<UStaticMesh>> RuntimeSpawnMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true"))
	bool bUseRuntimeSpawnMeshDirectory = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true"))
	FDirectoryPath RuntimeSpawnMeshDirectory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 RuntimeSpawnCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float RuntimeSpawnDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 RuntimeSpawnGridColumns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload Test", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float RuntimeSpawnSpacing = 180.0f;

	FTimerHandle RuntimeMeshSpawnTestTimerHandle;
};
