#include "LevelDataAssetGeneratorSettings.h"

#include "Misc/Paths.h"

namespace LevelDataAssetGeneratorSettings
{
	FString NormalizeContentDirectoryPath(const FString& RawPath)
	{
		FString Path = RawPath;
		Path.TrimStartAndEndInline();
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));

		if (Path.IsEmpty())
		{
			return TEXT("/Game");
		}

		if (Path.StartsWith(TEXT("/Game")))
		{
			Path.RemoveFromEnd(TEXT("/"));
			return Path;
		}

		FString FullPath = Path;
		if (FPaths::IsRelative(FullPath))
		{
			FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FullPath);
		}

		FPaths::NormalizeFilename(FullPath);

		FString RelativeContentPath = FullPath;
		if (FPaths::MakePathRelativeTo(RelativeContentPath, *FPaths::ProjectContentDir()))
		{
			RelativeContentPath.RemoveFromStart(TEXT("./"));
			RelativeContentPath.RemoveFromEnd(TEXT("/"));
			return RelativeContentPath.IsEmpty() ? TEXT("/Game") : FString::Printf(TEXT("/Game/%s"), *RelativeContentPath);
		}

		const FString ContentMarker = TEXT("/Content/");
		const int32 ContentIndex = FullPath.Find(ContentMarker, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (ContentIndex != INDEX_NONE)
		{
			const FString RelativePath = FullPath.Mid(ContentIndex + ContentMarker.Len());
			return RelativePath.IsEmpty() ? TEXT("/Game") : FString::Printf(TEXT("/Game/%s"), *RelativePath);
		}

		return Path;
	}

	void NormalizeContentDirectoryPaths(TArray<FDirectoryPath>& Paths)
	{
		for (FDirectoryPath& Path : Paths)
		{
			if (!Path.Path.IsEmpty())
			{
				Path.Path = NormalizeContentDirectoryPath(Path.Path);
			}
		}
	}
}

ULevelDataAssetGeneratorSettings::ULevelDataAssetGeneratorSettings()
{
	GeneratedDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTLevelPreloadDataAsset")));
	OutputFolder.Path = TEXT("/Game/Generated/LevelPreload");
	InventoryPreloadDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTInventoryPreloadDataAsset")));
	InventoryPreloadOutputFolder.Path = TEXT("/Game/Generated/LevelPreload");
	InventoryItemDataDirectory.Path = TEXT("/Game/Blueprints/Items/Data");
	InventoryItemDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTItemDataAsset")));

	FLevelDataAssetGeneratorFeatureDefinition CommonFeature;
	CommonFeature.FeatureName = TEXT("Common");
	CommonFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Loading") } };

	FLevelDataAssetGeneratorFeatureDefinition InventoryFeature;
	InventoryFeature.FeatureName = TEXT("Inventory");
	InventoryFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/Blueprints/Items/Data") }, FDirectoryPath{ TEXT("/Game/UI/Inventory") }, FDirectoryPath{ TEXT("/Game/UI/HUD") } };

	FLevelDataAssetGeneratorFeatureDefinition ShelfFeature;
	ShelfFeature.FeatureName = TEXT("Shelf");
	ShelfFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/Blueprints/Shelf") }, FDirectoryPath{ TEXT("/Game/Blueprints/Player") } };

	FLevelDataAssetGeneratorFeatureDefinition CombatFeature;
	CombatFeature.FeatureName = TEXT("Combat");
	CombatFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/Blueprints/Items/Data") }, FDirectoryPath{ TEXT("/Game/Blueprints/Items/GE") }, FDirectoryPath{ TEXT("/Game/Blueprints/Weapon") } };

	FLevelDataAssetGeneratorFeatureDefinition DamageTextFeature;
	DamageTextFeature.FeatureName = TEXT("DamageText");
	DamageTextFeature.DirectAssets = { TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/UI/WBP_DamageText.WBP_DamageText"))) };

	FLevelDataAssetGeneratorFeatureDefinition AIFeature;
	AIFeature.FeatureName = TEXT("AI");
	AIFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/Blueprints/NPC") } };

	FLevelDataAssetGeneratorFeatureDefinition CraftingFeature;
	CraftingFeature.FeatureName = TEXT("Crafting");
	CraftingFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Hub") } };

	FLevelDataAssetGeneratorFeatureDefinition ShopFeature;
	ShopFeature.FeatureName = TEXT("Shop");
	ShopFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Hub") } };

	FLevelDataAssetGeneratorFeatureDefinition MarketFeature;
	MarketFeature.FeatureName = TEXT("Market");
	MarketFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Hub") } };

	FLevelDataAssetGeneratorFeatureDefinition StorageFeature;
	StorageFeature.FeatureName = TEXT("Storage");
	StorageFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Hub") }, FDirectoryPath{ TEXT("/Game/UI/Inventory") }, FDirectoryPath{ TEXT("/Game/Blueprints/Hub") } };

	FLevelDataAssetGeneratorFeatureDefinition QuestFeature;
	QuestFeature.FeatureName = TEXT("Quest");
	QuestFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Hub") }, FDirectoryPath{ TEXT("/Game/UI/HUD") } };

	FLevelDataAssetGeneratorFeatureDefinition MainMenuFeature;
	MainMenuFeature.FeatureName = TEXT("MainMenu");
	MainMenuFeature.RootDirectories = { FDirectoryPath{ TEXT("/Game/UI/Menu") } };

	Features =
	{
		CommonFeature,
		InventoryFeature,
		ShelfFeature,
		CombatFeature,
		DamageTextFeature,
		AIFeature,
		CraftingFeature,
		ShopFeature,
		MarketFeature,
		StorageFeature,
		QuestFeature,
		MainMenuFeature
	};

	FLevelDataAssetGeneratorPresetDefinition PlayPreset;
	PlayPreset.PresetName = TEXT("Play");
	PlayPreset.LevelIds = { TEXT("Lvl_Main"), TEXT("Market_Test") };
	PlayPreset.FeatureNames = { TEXT("Common"), TEXT("Inventory"), TEXT("Shelf"), TEXT("Combat"), TEXT("DamageText"), TEXT("AI"), TEXT("Quest") };
	PlayPreset.bAssignInventoryPreloadDataAsset = true;

	FLevelDataAssetGeneratorPresetDefinition HubPreset;
	HubPreset.PresetName = TEXT("Hub");
	HubPreset.LevelIds = { TEXT("Lvl_Hub") };
	HubPreset.FeatureNames = { TEXT("Common"), TEXT("Inventory"), TEXT("Crafting"), TEXT("Shop"), TEXT("Market"), TEXT("Storage"), TEXT("Quest") };
	HubPreset.bAssignInventoryPreloadDataAsset = true;

	FLevelDataAssetGeneratorPresetDefinition MainMenuPreset;
	MainMenuPreset.PresetName = TEXT("MainMenu");
	MainMenuPreset.LevelIds = { TEXT("Lvl_MainMenu") };
	MainMenuPreset.FeatureNames = { TEXT("MainMenu") };

	Presets = { PlayPreset, HubPreset, MainMenuPreset };
}

#if WITH_EDITOR
void ULevelDataAssetGeneratorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OutputFolder.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(OutputFolder.Path);
	InventoryPreloadOutputFolder.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(InventoryPreloadOutputFolder.Path);
	InventoryItemDataDirectory.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(InventoryItemDataDirectory.Path);
	for (FLevelDataAssetGeneratorFeatureDefinition& Feature : Features)
	{
		LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPaths(Feature.RootDirectories);
	}

	for (FDirectoryPath& IgnoredPath : IgnoredPaths)
	{
		if (!IgnoredPath.Path.IsEmpty())
		{
			IgnoredPath.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(IgnoredPath.Path);
		}
	}
}
#endif
