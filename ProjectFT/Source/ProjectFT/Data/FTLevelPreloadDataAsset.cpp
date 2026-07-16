#include "ProjectFT/Data/FTLevelPreloadDataAsset.h"

#include "ProjectFT/Data/FTItemDataAsset.h"

const FPrimaryAssetType UFTLevelPreloadDataAsset::AssetType = TEXT("FTLevelPreload");

FPrimaryAssetId UFTLevelPreloadDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

void UFTLevelPreloadDataAsset::GetPreloadAssetPaths(TArray<FSoftObjectPath>& OutAssetPaths) const
{
	TSet<FString> ExcludedPathStrings;
	GetExcludedAssetPaths(ExcludedPathStrings);

	TSet<FString> AddedPathStrings;
	auto AddAssetPath = [&OutAssetPaths, &AddedPathStrings, &ExcludedPathStrings](const FSoftObjectPath& AssetPath)
	{
		const FString AssetPathString = AssetPath.ToString();
		if (AssetPath.IsValid() && !ExcludedPathStrings.Contains(AssetPathString) && !AddedPathStrings.Contains(AssetPathString))
		{
			OutAssetPaths.Add(AssetPath);
			AddedPathStrings.Add(AssetPathString);
		}
	};

	for (const TSoftObjectPtr<UObject>& Asset : GeneratedEnvironmentAssets)
	{
		AddAssetPath(Asset.ToSoftObjectPath());
	}

	for (const TSoftObjectPtr<UObject>& Asset : GeneratedRuntimeAssets)
	{
		AddAssetPath(Asset.ToSoftObjectPath());
	}

	for (const TSoftObjectPtr<UObject>& Asset : AdditionalPreloadAssets)
	{
		AddAssetPath(Asset.ToSoftObjectPath());
	}
}

void UFTLevelPreloadDataAsset::GetExcludedAssetPaths(TSet<FString>& OutExcludedPathStrings) const
{
	for (const TSoftObjectPtr<UObject>& ExcludedAsset : ExcludedAssets)
	{
		const FSoftObjectPath ExcludedPath = ExcludedAsset.ToSoftObjectPath();
		if (ExcludedPath.IsValid())
		{
			OutExcludedPathStrings.Add(ExcludedPath.ToString());
		}
	}
}
