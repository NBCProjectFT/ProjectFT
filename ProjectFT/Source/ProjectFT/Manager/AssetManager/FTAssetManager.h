#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Templates/SubclassOf.h"
#include "FTAssetManager.generated.h"

class UFTGameDataAsset;
class UFTInventoryPreloadDataAsset;
class UFTLevelPreloadDataAsset;
class UFTEscapedRaidWidget;
class UFTFailWidget;
class UFTHubCraftWidget;
class UFTHubStorageWidget;
class UFTCountdownEscapeWidget;
class UFTInventoryWidget;
class UFTLoadingWidget;
class UFTMainHUDWidget;
class UFTMainMenuWidget;
class UFTPauseMenuWidget;
class UFTQuestListWidget;
class UMaterialInterface;
class UUserWidget;
struct FStreamableHandle;

DECLARE_DELEGATE_ThreeParams(FFTAssetLoadProgressDelegate, const FString&, int32, int32);

UCLASS(Config = Game)
class PROJECTFT_API UFTAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UFTAssetManager& Get();

	template <typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template <typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template <typename AssetType>
	void LoadAssetAsync(const TSoftObjectPtr<AssetType>& AssetPointer, TFunction<void(AssetType*)> OnLoaded, bool bKeepInMemory = true);

	template <typename AssetType>
	void LoadSubclassAsync(const TSoftClassPtr<AssetType>& AssetPointer, TFunction<void(TSubclassOf<AssetType>)> OnLoaded, bool bKeepInMemory = true);

	void PreloadLevelAssetsAsync(
		const TSoftObjectPtr<UFTLevelPreloadDataAsset>& LevelPreloadDataAsset,
		FSimpleDelegate OnLoaded,
		FFTAssetLoadProgressDelegate OnProgress = FFTAssetLoadProgressDelegate());

	static void DumpLoadedAssets();

	const UFTGameDataAsset* GetGameData();
	TSubclassOf<UFTLoadingWidget> GetLoadingWidgetClass();
	TSubclassOf<UFTMainMenuWidget> GetMainMenuWidgetClass();
	TSubclassOf<UFTMainHUDWidget> GetMainHUDWidgetClass();
	TSubclassOf<UFTInventoryWidget> GetInventoryWidgetClass();
	TSubclassOf<UFTPauseMenuWidget> GetPauseMenuWidgetClass();
	TSubclassOf<UFTQuestListWidget> GetQuestListWidgetClass();
	TSubclassOf<UFTHubStorageWidget> GetHubStorageWidgetClass();
	TSubclassOf<UFTHubCraftWidget> GetHubCraftWidgetClass();
	TSubclassOf<UFTCountdownEscapeWidget> GetCountdownEscapeWidgetClass();
	TSubclassOf<UFTEscapedRaidWidget> GetEscapedRaidWidgetClass();
	TSubclassOf<UFTFailWidget> GetFailWidgetClass();
	TSubclassOf<UUserWidget> GetDamageTextWidgetClass();
	UMaterialInterface* GetDamageTextBackgroundMaterial();
	UMaterialInterface* GetPaperFlutterMaterial();

protected:
	virtual void StartInitialLoading() override;

private:
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	static bool ShouldLogAssetLoads();

	void AddLoadedAsset(const UObject* Asset);
	UFTGameDataAsset* LoadGameData();
	TArray<FSoftObjectPath> CollectLevelPreloadAssetPaths(const UFTLevelPreloadDataAsset& LevelPreloadData) const;
	TArray<FSoftObjectPath> CollectInventoryPreloadAssetPaths(const UFTInventoryPreloadDataAsset& InventoryPreloadData) const;
	TArray<FSoftObjectPath> CollectAllInventoryItemPreloadAssetPaths() const;
	void AppendPrimaryAssetPaths(FPrimaryAssetType AssetType, TArray<FSoftObjectPath>& AssetPaths) const;
	void AddUniqueAssetPath(TArray<FSoftObjectPath>& AssetPaths, TSet<FString>& AddedAssetPathStrings, const FSoftObjectPath& AssetPath) const;
	void LoadPreloadPathQueue(TArray<FSoftObjectPath> PendingPaths, int32 CompletedCount, int32 TotalCount, FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress);
	void HandlePreloadPathLoaded(FSoftObjectPath LoadedPath, TArray<FSoftObjectPath> PendingPaths, int32 CompletedCount, int32 TotalCount, FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress);
	void AddLoadedAssets(const TArray<FSoftObjectPath>& LoadedPaths);
	void RetainPreloadedAssetsForNextPreload(const TArray<FSoftObjectPath>& NextPreloadAssetPaths);
	void RemoveCompletedLoadHandles();

private:
	UPROPERTY(Config)
	TSoftObjectPtr<UFTGameDataAsset> GameDataPath;

	UPROPERTY(Transient)
	TObjectPtr<UFTGameDataAsset> GameData = nullptr;

	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	TArray<TSharedPtr<FStreamableHandle>> ActiveLoadHandles;

	TSet<FString> ActivePreloadAssetPathStrings;

	FCriticalSection LoadedAssetsCritical;
};

template <typename AssetType>
AssetType* UFTAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;
	const FSoftObjectPath AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
		}

		if (LoadedAsset && bKeepInMemory)
		{
			Get().AddLoadedAsset(LoadedAsset);
		}
	}

	return LoadedAsset;
}

template <typename AssetType>
TSubclassOf<AssetType> UFTAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;
	const FSoftObjectPath AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			Get().AddLoadedAsset(LoadedSubclass);
		}
	}

	return LoadedSubclass;
}

template <typename AssetType>
void UFTAssetManager::LoadAssetAsync(const TSoftObjectPtr<AssetType>& AssetPointer, TFunction<void(AssetType*)> OnLoaded, bool bKeepInMemory)
{
	const FSoftObjectPath AssetPath = AssetPointer.ToSoftObjectPath();
	if (!AssetPath.IsValid())
	{
		OnLoaded(nullptr);
		return;
	}

	GetStreamableManager().RequestAsyncLoad(AssetPath, [AssetPath, OnLoaded, bKeepInMemory]()
	{
		AssetType* LoadedAsset = Cast<AssetType>(AssetPath.ResolveObject());
		if (LoadedAsset && bKeepInMemory)
		{
			UFTAssetManager::Get().AddLoadedAsset(LoadedAsset);
		}

		OnLoaded(LoadedAsset);
	});
}

template <typename AssetType>
void UFTAssetManager::LoadSubclassAsync(const TSoftClassPtr<AssetType>& AssetPointer, TFunction<void(TSubclassOf<AssetType>)> OnLoaded, bool bKeepInMemory)
{
	const FSoftObjectPath AssetPath = AssetPointer.ToSoftObjectPath();
	if (!AssetPath.IsValid())
	{
		OnLoaded(nullptr);
		return;
	}

	GetStreamableManager().RequestAsyncLoad(AssetPath, [AssetPath, OnLoaded, bKeepInMemory]()
	{
		TSubclassOf<AssetType> LoadedSubclass = Cast<UClass>(AssetPath.ResolveObject());
		if (LoadedSubclass && bKeepInMemory)
		{
			UFTAssetManager::Get().AddLoadedAsset(LoadedSubclass);
		}

		OnLoaded(LoadedSubclass);
	});
}
