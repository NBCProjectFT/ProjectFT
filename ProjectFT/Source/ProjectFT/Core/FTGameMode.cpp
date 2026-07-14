// Fill out your copyright notice in the Description page of Project Settings.


#include "FTGameMode.h"

#include "FTSaveSubsystem.h"
#include "FTGameFlowSubsystem.h"
#include "FTLogChannels.h"
#include "../Manager/AssetManager/FTAssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "TimerManager.h"

AFTGameMode::AFTGameMode()
{
	FTStorageItemStruct TestItem;
	TestItem.ItemID = TEXT("AK47");
	TestItem.Count = 1;
	RaidStartTestItems.Add(TestItem);
}

void AFTGameMode::StartPlay()
{
	Super::StartPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->SyncFlowStateWithCurrentLevel();

			if (UFTSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UFTSaveSubsystem>())
			{
				SaveSubsystem->RestoreCurrentWorldState();
			}

			GrantRaidStartTestItemsIfNeeded(GameFlowSubsystem);
		}
	}

	// if (bEnableRuntimeMeshSpawnTest)
	// {
	// 	GetWorldTimerManager().SetTimer(RuntimeMeshSpawnTestTimerHandle, this, &ThisClass::SpawnRuntimeMeshTest, RuntimeSpawnDelay, false);
	// }
}

void AFTGameMode::HandleRaidStart()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestStartRaid();
		}
	}
}

void AFTGameMode::HandleRaidFail()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestFailRaid();
		}
	}
}

void AFTGameMode::HandleRaidEscape()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestEscapeRaid();
		}
	}
}

void AFTGameMode::GrantRaidStartTestItemsIfNeeded(const UFTGameFlowSubsystem* GameFlowSubsystem) const
{
	if (!bGrantRaidStartTestItems || !GameFlowSubsystem || GameFlowSubsystem->GetCurrentFlowState() != EFTFlowStateType::RaidInProgress)
	{
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	UFTInventoryComponent* PlayerInventory = PlayerPawn->FindComponentByClass<UFTInventoryComponent>();
	if (!PlayerInventory || !PlayerInventory->GetItems().IsEmpty())
	{
		return;
	}

	for (const FTStorageItemStruct& TestItem : RaidStartTestItems)
	{
		if (TestItem.ItemID.IsNone() || TestItem.Count <= 0)
		{
			continue;
		}

		PlayerInventory->AddItem(TestItem.ItemID, TestItem.Count);
	}
}

void AFTGameMode::SpawnRuntimeMeshTest()
{
	const TArray<TSoftObjectPtr<UStaticMesh>> MeshesToLoad = CollectRuntimeSpawnMeshes();
	const double LoadStartTime = FPlatformTime::Seconds();
	TArray<UStaticMesh*> LoadedMeshes;
	LoadedMeshes.Reserve(MeshesToLoad.Num());

	for (const TSoftObjectPtr<UStaticMesh>& RuntimeSpawnMesh : MeshesToLoad)
	{
		UStaticMesh* LoadedMesh = UFTAssetManager::GetAsset(RuntimeSpawnMesh);
		if (LoadedMesh)
		{
			LoadedMeshes.Add(LoadedMesh);
		}
		else
		{
			UE_LOG(LogFTAsset, Warning, TEXT("Runtime mesh spawn test failed to load mesh: %s"), *RuntimeSpawnMesh.ToString());
		}
	}

	const double LoadEndTime = FPlatformTime::Seconds();
	if (LoadedMeshes.IsEmpty())
	{
		UE_LOG(LogFTAsset, Warning, TEXT("Runtime mesh spawn test stopped because no meshes were loaded."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double SpawnStartTime = FPlatformTime::Seconds();
	const int32 ColumnCount = FMath::Max(RuntimeSpawnGridColumns, 1);
	const FVector StartLocation(0.0f, 0.0f, 100.0f);

	for (int32 Index = 0; Index < RuntimeSpawnCount; ++Index)
	{
		const int32 Row = Index / ColumnCount;
		const int32 Column = Index % ColumnCount;
		const FVector SpawnLocation = StartLocation + FVector(Row * RuntimeSpawnSpacing, Column * RuntimeSpawnSpacing, 0.0f);

		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator);
		if (!MeshActor)
		{
			continue;
		}

		UStaticMeshComponent* StaticMeshComponent = MeshActor->GetStaticMeshComponent();
		if (StaticMeshComponent)
		{
			UStaticMesh* SpawnMesh = LoadedMeshes[Index % LoadedMeshes.Num()];
			StaticMeshComponent->SetMobility(EComponentMobility::Movable);
			StaticMeshComponent->SetStaticMesh(SpawnMesh);
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	const double SpawnEndTime = FPlatformTime::Seconds();
	UE_LOG(LogFTAsset, Warning, TEXT("Runtime mesh spawn test loaded %d unique meshes in %.3f ms."), LoadedMeshes.Num(), (LoadEndTime - LoadStartTime) * 1000.0);
	UE_LOG(LogFTAsset, Warning, TEXT("Runtime mesh spawn test spawned %d actors in %.3f ms."), RuntimeSpawnCount, (SpawnEndTime - SpawnStartTime) * 1000.0);
}

TArray<TSoftObjectPtr<UStaticMesh>> AFTGameMode::CollectRuntimeSpawnMeshes() const
{
	if (!bUseRuntimeSpawnMeshDirectory || RuntimeSpawnMeshDirectory.Path.IsEmpty())
	{
		return RuntimeSpawnMeshes;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*RuntimeSpawnMeshDirectory.Path));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());

	TArray<FAssetData> FoundAssets;
	AssetRegistry.GetAssets(Filter, FoundAssets);

	TArray<TSoftObjectPtr<UStaticMesh>> Result;
	Result.Reserve(FoundAssets.Num());

	for (const FAssetData& AssetData : FoundAssets)
	{
		Result.Add(TSoftObjectPtr<UStaticMesh>(AssetData.ToSoftObjectPath()));
	}

	UE_LOG(LogFTAsset, Warning, TEXT("Runtime mesh spawn test collected %d static meshes from %s."), Result.Num(), *RuntimeSpawnMeshDirectory.Path);
	return Result;
}
