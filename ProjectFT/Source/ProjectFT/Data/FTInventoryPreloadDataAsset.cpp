#include "ProjectFT/Data/FTInventoryPreloadDataAsset.h"

#include "ProjectFT/Data/FTItemDataAsset.h"

const FPrimaryAssetType UFTInventoryPreloadDataAsset::AssetType = TEXT("FTInventoryPreload");

FPrimaryAssetId UFTInventoryPreloadDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

void UFTInventoryPreloadDataAsset::GetPreloadAssetPaths(TArray<FSoftObjectPath>& OutAssetPaths) const
{
	TSet<FString> ExcludedPathStrings;
	GetExcludedAssetPaths(ExcludedPathStrings);

	TSet<FString> AddedPathStrings;
	for (const FSoftObjectPath& ExistingPath : OutAssetPaths)
	{
		if (ExistingPath.IsValid())
		{
			AddedPathStrings.Add(ExistingPath.ToString());
		}
	}

	auto AddAssetPath = [&OutAssetPaths, &AddedPathStrings, &ExcludedPathStrings](const FSoftObjectPath& AssetPath)
	{
		const FString AssetPathString = AssetPath.ToString();
		if (AssetPath.IsValid() && !ExcludedPathStrings.Contains(AssetPathString) && !AddedPathStrings.Contains(AssetPathString))
		{
			OutAssetPaths.Add(AssetPath);
			AddedPathStrings.Add(AssetPathString);
		}
	};

	for (const TSoftObjectPtr<UFTItemDataAsset>& Asset : InventoryItemAssets)
	{
		AddAssetPath(Asset.ToSoftObjectPath());
	}

	for (const TSoftObjectPtr<UObject>& Asset : AdditionalPreloadAssets)
	{
		AddAssetPath(Asset.ToSoftObjectPath());
	}
}

void UFTInventoryPreloadDataAsset::GetExcludedAssetPaths(TSet<FString>& OutExcludedPathStrings) const
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
