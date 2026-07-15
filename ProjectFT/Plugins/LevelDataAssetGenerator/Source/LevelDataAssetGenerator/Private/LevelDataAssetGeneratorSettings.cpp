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
}

ULevelDataAssetGeneratorSettings::ULevelDataAssetGeneratorSettings()
{
	GeneratedDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTLevelPreloadDataAsset")));
	OutputFolder.Path = TEXT("/Game/Generated/LevelPreload");
	InventoryPreloadDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTInventoryPreloadDataAsset")));
	InventoryPreloadOutputFolder.Path = TEXT("/Game/Generated/LevelPreload");
	InventoryItemDataDirectory.Path = TEXT("/Game/Blueprints/Items/Data");
	InventoryItemDataAssetClass = TSoftClassPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT("/Script/ProjectFT.FTItemDataAsset")));
}

#if WITH_EDITOR
void ULevelDataAssetGeneratorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OutputFolder.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(OutputFolder.Path);
	InventoryPreloadOutputFolder.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(InventoryPreloadOutputFolder.Path);
	InventoryItemDataDirectory.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(InventoryItemDataDirectory.Path);

	for (FDirectoryPath& IgnoredPath : IgnoredPaths)
	{
		if (!IgnoredPath.Path.IsEmpty())
		{
			IgnoredPath.Path = LevelDataAssetGeneratorSettings::NormalizeContentDirectoryPath(IgnoredPath.Path);
		}
	}
}
#endif
