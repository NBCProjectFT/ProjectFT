#include "FTHubItemDataResolver.h"

#include "Engine/AssetManager.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

namespace
{
	const FPrimaryAssetType ItemAssetType(TEXT("FTItemItem"));
	const FString ItemDataPath(TEXT("/Game/Blueprints/Items/Data"));

	void EnsureItemAssetsScanned(UAssetManager& AssetManager)
	{
		static bool bItemAssetsScanned = false;
		if (bItemAssetsScanned)
		{
			return;
		}

		TArray<FString> Paths;
		Paths.Add(ItemDataPath);
		AssetManager.ScanPathsForPrimaryAssets(ItemAssetType, Paths, UFTItemDataAsset::StaticClass(), false, true);
		bItemAssetsScanned = true;
	}

	UFTItemDataAsset* LoadItemDataAsset(UAssetManager& AssetManager, const FPrimaryAssetId& AssetID)
	{
		if (!AssetID.IsValid())
		{
			return nullptr;
		}

		UObject* AssetObject = AssetManager.GetPrimaryAssetObject(AssetID);
		if (!AssetObject)
		{
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetID);
			if (AssetPath.IsValid())
			{
				AssetObject = AssetPath.TryLoad();
			}
		}

		return Cast<UFTItemDataAsset>(AssetObject);
	}
}

const UFTItemDataAsset* FTHubItemDataResolver::FindItemData(const FName ItemID)
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}

	static TMap<FName, TWeakObjectPtr<UFTItemDataAsset>> CachedItemData;
	if (const TWeakObjectPtr<UFTItemDataAsset>* CachedItemDataPtr = CachedItemData.Find(ItemID))
	{
		if (CachedItemDataPtr->IsValid())
		{
			return CachedItemDataPtr->Get();
		}
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	EnsureItemAssetsScanned(AssetManager);

	if (UFTItemDataAsset* ExactItemData = LoadItemDataAsset(AssetManager, FPrimaryAssetId(ItemAssetType, ItemID)))
	{
		CachedItemData.Add(ItemID, ExactItemData);
		return ExactItemData;
	}

	TArray<FPrimaryAssetId> ItemAssetIds;
	AssetManager.GetPrimaryAssetIdList(ItemAssetType, ItemAssetIds);

	for (const FPrimaryAssetId& AssetID : ItemAssetIds)
	{
		UFTItemDataAsset* ItemDataAsset = LoadItemDataAsset(AssetManager, AssetID);
		if (ItemDataAsset && ItemDataAsset->ItemData.ItemId == ItemID)
		{
			CachedItemData.Add(ItemID, ItemDataAsset);
			return ItemDataAsset;
		}
	}

	return nullptr;
}
