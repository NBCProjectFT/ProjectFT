#include "FTGenerateLevelPreloadDataCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTLevelPreloadDataAsset.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* DefaultLevelName = TEXT("Lvl_Main");
	const TCHAR* DefaultLevelDirectory = TEXT("/Game/Level");
	const TCHAR* DefaultItemDataDirectory = TEXT("/Game/Blueprints/Items/Data");
	const TCHAR* DefaultOutputDirectory = TEXT("/Game/Blueprints/LevelPreload");

	FString NormalizeContentPath(FString Path)
	{
		Path.TrimStartAndEndInline();
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		Path.RemoveFromEnd(TEXT("/"));
		return Path;
	}

	FString BuildObjectPath(const FString& PackageName, const FString& ObjectName)
	{
		return FString::Printf(TEXT("%s.%s"), *PackageName, *ObjectName);
	}
}

UFTGenerateLevelPreloadDataCommandlet::UFTGenerateLevelPreloadDataCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFTGenerateLevelPreloadDataCommandlet::Main(const FString& Params)
{
	FString LevelName = DefaultLevelName;
	FParse::Value(*Params, TEXT("LevelName="), LevelName);

	FString LevelDirectory = DefaultLevelDirectory;
	FParse::Value(*Params, TEXT("LevelDirectory="), LevelDirectory);
	LevelDirectory = NormalizeContentPath(LevelDirectory);

	FString ItemDataDirectory = DefaultItemDataDirectory;
	FParse::Value(*Params, TEXT("ItemDataDirectory="), ItemDataDirectory);
	ItemDataDirectory = NormalizeContentPath(ItemDataDirectory);

	FString OutputDirectory = DefaultOutputDirectory;
	FParse::Value(*Params, TEXT("OutputDirectory="), OutputDirectory);
	OutputDirectory = NormalizeContentPath(OutputDirectory);

	if (LevelName.IsEmpty() || ItemDataDirectory.IsEmpty() || OutputDirectory.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid params. LevelName, ItemDataDirectory, and OutputDirectory must not be empty."));
		return 1;
	}

	const FString AssetName = FString::Printf(TEXT("DA_LevelPreload_%s"), *LevelName);
	const FString PackageName = FString::Printf(TEXT("%s/%s"), *OutputDirectory, *AssetName);
	const FString ObjectPath = BuildObjectPath(PackageName, AssetName);

	UFTLevelPreloadDataAsset* DataAsset = LoadObject<UFTLevelPreloadDataAsset>(nullptr, *ObjectPath);
	UPackage* Package = DataAsset ? DataAsset->GetOutermost() : CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
		return 1;
	}

	if (!DataAsset)
	{
		DataAsset = NewObject<UFTLevelPreloadDataAsset>(Package, UFTLevelPreloadDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(DataAsset);
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*ItemDataDirectory));
	Filter.ClassPaths.Add(UFTItemDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	TArray<FAssetData> ItemAssetDataList;
	AssetRegistry.GetAssets(Filter, ItemAssetDataList);
	ItemAssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.ToSoftObjectPath().ToString() < Right.ToSoftObjectPath().ToString();
	});

	DataAsset->Modify();
	DataAsset->LevelId = FName(*LevelName);
	DataAsset->Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(BuildObjectPath(FString::Printf(TEXT("%s/%s"), *LevelDirectory, *LevelName), LevelName)));
	DataAsset->GeneratedInventoryItemAssets.Reset();
	DataAsset->GeneratedInventoryItemAssets.Reserve(ItemAssetDataList.Num());
	DataAsset->bPreloadAllInventoryItemDataAssets = true;

	for (const FAssetData& ItemAssetData : ItemAssetDataList)
	{
		DataAsset->GeneratedInventoryItemAssets.Add(TSoftObjectPtr<UFTItemDataAsset>(ItemAssetData.ToSoftObjectPath()));
	}

	Package->MarkPackageDirty();

	const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(Package, DataAsset, *PackageFilename, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save level preload DataAsset: %s"), *ObjectPath);
		return 1;
	}

	UE_LOG(LogTemp, Log, TEXT("Generated level preload DataAsset: %s"), *ObjectPath);
	UE_LOG(LogTemp, Log, TEXT("Inventory item preload assets: %d"), DataAsset->GeneratedInventoryItemAssets.Num());
	return 0;
}
