#include "FTAssetManager.h"

#include "../../Core/FTLogChannels.h"
#include "../../Data/FTGameDataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"

UFTAssetManager& UFTAssetManager::Get()
{
	check(GEngine);

	if (UFTAssetManager* AssetManager = Cast<UFTAssetManager>(GEngine->AssetManager))
	{
		return *AssetManager;
	}

	UE_LOG(LogFTAsset, Fatal, TEXT("Invalid AssetManagerClassName. It must be set to FTAssetManager."));
	return *NewObject<UFTAssetManager>();
}

void UFTAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadGameData();
}

void UFTAssetManager::PreloadGameDataAssetsAsync(FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress)
{
	UFTGameDataAsset* LoadedGameData = LoadGameData();
	if (!LoadedGameData)
	{
		OnLoaded.ExecuteIfBound();
		return;
	}

	TArray<FSoftObjectPath> AssetPaths = CollectPreloadAssetPaths(*LoadedGameData);

	if (AssetPaths.IsEmpty())
	{
		UE_LOG(LogFTAsset, Warning, TEXT("No assets to preload."));
		OnLoaded.ExecuteIfBound();
		return;
	}

	UE_LOG(LogFTAsset, Log, TEXT("Preloading %d assets from game data preload settings."), AssetPaths.Num());

	LoadPreloadPathQueue(AssetPaths, 0, AssetPaths.Num(), OnLoaded, OnProgress);
}

UObject* UFTAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	if (ShouldLogAssetLoads())
	{
		UE_LOG(LogFTAsset, Log, TEXT("Synchronously loading asset: %s"), *AssetPath.ToString());
	}

	if (UAssetManager::IsInitialized())
	{
		return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
	}

	return AssetPath.TryLoad();
}

bool UFTAssetManager::ShouldLogAssetLoads()
{
	static const bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UFTAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (!ensureAlways(Asset))
	{
		return;
	}

	FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
	LoadedAssets.Add(Asset);
}

TArray<FSoftObjectPath> UFTAssetManager::CollectPreloadAssetPaths(const UFTGameDataAsset& LoadedGameData) const
{
	TArray<FSoftObjectPath> AssetPaths;
	AppendDirectoryAssetPaths(LoadedGameData.PreloadDirectories, AssetPaths);
	AppendManualAssetPaths(LoadedGameData.PreloadAssets, AssetPaths);
	RemoveExcludedAssetPaths(LoadedGameData, AssetPaths);
	return AssetPaths;
}

void UFTAssetManager::AppendDirectoryAssetPaths(const TArray<FDirectoryPath>& Directories, TArray<FSoftObjectPath>& AssetPaths) const
{
	if (Directories.IsEmpty())
	{
		return;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	FARFilter Filter;
	Filter.bRecursivePaths = true;

	for (const FDirectoryPath& Directory : Directories)
	{
		if (!Directory.Path.IsEmpty())
		{
			Filter.PackagePaths.Add(FName(*Directory.Path));
		}
	}

	TArray<FAssetData> FoundAssets;
	AssetRegistry.GetAssets(Filter, FoundAssets);

	TSet<FString> AddedAssetPathStrings;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		AddedAssetPathStrings.Add(AssetPath.ToString());
	}

	for (const FAssetData& AssetData : FoundAssets)
	{
		AddUniqueAssetPath(AssetPaths, AddedAssetPathStrings, AssetData.ToSoftObjectPath());
	}
}

void UFTAssetManager::AppendManualAssetPaths(const TArray<TSoftObjectPtr<UObject>>& Assets, TArray<FSoftObjectPath>& AssetPaths) const
{
	TSet<FString> AddedAssetPathStrings;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		AddedAssetPathStrings.Add(AssetPath.ToString());
	}

	for (const TSoftObjectPtr<UObject>& Asset : Assets)
	{
		AddUniqueAssetPath(AssetPaths, AddedAssetPathStrings, Asset.ToSoftObjectPath());
	}
}

void UFTAssetManager::RemoveExcludedAssetPaths(const UFTGameDataAsset& LoadedGameData, TArray<FSoftObjectPath>& AssetPaths) const
{
	AssetPaths.RemoveAll([this, &LoadedGameData](const FSoftObjectPath& AssetPath)
	{
		return IsAssetPathExcluded(AssetPath, LoadedGameData);
	});
}

bool UFTAssetManager::IsAssetPathExcluded(const FSoftObjectPath& AssetPath, const UFTGameDataAsset& LoadedGameData) const
{
	const FString AssetPathString = AssetPath.ToString();

	for (const TSoftObjectPtr<UObject>& ExcludedAsset : LoadedGameData.ExcludedAssets)
	{
		const FSoftObjectPath ExcludedPath = ExcludedAsset.ToSoftObjectPath();
		if (ExcludedPath.IsValid() && ExcludedPath.ToString() == AssetPathString)
		{
			return true;
		}
	}

	for (const FDirectoryPath& ExcludedDirectory : LoadedGameData.ExcludedDirectories)
	{
		if (IsPathInDirectory(AssetPathString, ExcludedDirectory.Path))
		{
			return true;
		}
	}

	return false;
}

bool UFTAssetManager::IsPathInDirectory(const FString& AssetPath, const FString& DirectoryPath) const
{
	if (DirectoryPath.IsEmpty())
	{
		return false;
	}

	FString NormalizedDirectoryPath = DirectoryPath;
	NormalizedDirectoryPath.RemoveFromEnd(TEXT("/"));

	return AssetPath == NormalizedDirectoryPath || AssetPath.StartsWith(NormalizedDirectoryPath + TEXT("/"));
}

void UFTAssetManager::AddUniqueAssetPath(TArray<FSoftObjectPath>& AssetPaths, TSet<FString>& AddedAssetPathStrings, const FSoftObjectPath& AssetPath) const
{
	if (!AssetPath.IsValid())
	{
		return;
	}

	const FString AssetPathString = AssetPath.ToString();
	if (AddedAssetPathStrings.Contains(AssetPathString))
	{
		return;
	}

	AssetPaths.Add(AssetPath);
	AddedAssetPathStrings.Add(AssetPathString);
}

void UFTAssetManager::LoadPreloadPathQueue(TArray<FSoftObjectPath> PendingPaths, int32 CompletedCount, int32 TotalCount, FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress)
{
	if (PendingPaths.IsEmpty())
	{
		RemoveCompletedLoadHandles();
		OnLoaded.ExecuteIfBound();
		return;
	}

	const FSoftObjectPath LoadingPath = PendingPaths[0];
	PendingPaths.RemoveAt(0);

	OnProgress.ExecuteIfBound(LoadingPath.GetAssetName(), CompletedCount, TotalCount);

	TSharedPtr<FStreamableHandle> LoadHandle = GetStreamableManager().RequestAsyncLoad(
		LoadingPath,
		FStreamableDelegate::CreateUObject(this, &UFTAssetManager::HandlePreloadPathLoaded, LoadingPath, PendingPaths, CompletedCount, TotalCount, OnLoaded, OnProgress)
	);

	ActiveLoadHandles.Add(LoadHandle);
}

void UFTAssetManager::HandlePreloadPathLoaded(FSoftObjectPath LoadedPath, TArray<FSoftObjectPath> PendingPaths, int32 CompletedCount, int32 TotalCount, FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress)
{
	TArray<FSoftObjectPath> LoadedPaths;
	LoadedPaths.Add(LoadedPath);
	AddLoadedAssets(LoadedPaths);
	++CompletedCount;

	OnProgress.ExecuteIfBound(LoadedPath.GetAssetName(), CompletedCount, TotalCount);
	RemoveCompletedLoadHandles();
	LoadPreloadPathQueue(PendingPaths, CompletedCount, TotalCount, OnLoaded, OnProgress);
}

void UFTAssetManager::AddLoadedAssets(const TArray<FSoftObjectPath>& LoadedPaths)
{
	for (const FSoftObjectPath& LoadedPath : LoadedPaths)
	{
		if (const UObject* LoadedAsset = LoadedPath.ResolveObject())
		{
			AddLoadedAsset(LoadedAsset);
		}
		else
		{
			UE_LOG(LogFTAsset, Warning, TEXT("Preloaded asset did not resolve: %s"), *LoadedPath.ToString());
		}
	}
}

void UFTAssetManager::RemoveCompletedLoadHandles()
{
	ActiveLoadHandles.RemoveAll([](const TSharedPtr<FStreamableHandle>& LoadHandle)
	{
		return !LoadHandle.IsValid() || LoadHandle->HasLoadCompleted();
	});
}

void UFTAssetManager::DumpLoadedAssets()
{
	UFTAssetManager& AssetManager = Get();

	UE_LOG(LogFTAsset, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	FScopeLock LoadedAssetsLock(&AssetManager.LoadedAssetsCritical);
	for (const UObject* LoadedAsset : AssetManager.LoadedAssets)
	{
		UE_LOG(LogFTAsset, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogFTAsset, Log, TEXT("... %d assets in loaded pool"), AssetManager.LoadedAssets.Num());
	UE_LOG(LogFTAsset, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

const UFTGameDataAsset* UFTAssetManager::GetGameData()
{
	return LoadGameData();
}

UFTGameDataAsset* UFTAssetManager::LoadGameData()
{
	if (GameData)
	{
		return GameData;
	}

	if (GameDataPath.IsNull())
	{
		UE_LOG(LogFTAsset, Warning, TEXT("GameDataPath is not set."));
		return nullptr;
	}

	GameData = GetAsset(GameDataPath);
	if (!GameData)
	{
		UE_LOG(LogFTAsset, Error, TEXT("Failed to load game data asset: %s"), *GameDataPath.ToString());
	}

	return GameData;
}
