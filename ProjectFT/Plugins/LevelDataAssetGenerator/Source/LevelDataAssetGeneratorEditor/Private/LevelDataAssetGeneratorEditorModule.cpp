#include "LevelDataAssetGeneratorSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "HAL/IConsoleManager.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "LevelDataAssetGeneratorEditor"

namespace LevelDataAssetGeneratorEditor
{
	enum class ELevelPreloadEntrySourceType : uint8
	{
		Environment,
		InventoryItem,
		RuntimeFeature
	};

	struct FLevelPreloadEntry
	{
		FSoftObjectPath AssetPath;
		FTopLevelAssetPath AssetClassPath;
		FString SourceDescription;
		ELevelPreloadEntrySourceType SourceType = ELevelPreloadEntrySourceType::Environment;
		bool bIncluded = true;
	};

	struct FScanResult
	{
		FString LevelPackageName;
		FSoftObjectPath LevelObjectPath;
		FName PresetName = NAME_None;
		TArray<TSharedPtr<FLevelPreloadEntry>> Entries;
	};

	FString NormalizeContentDirectoryPath(const FString& RawPath);

	bool IsPathIgnored(const FString& PackageName, const ULevelDataAssetGeneratorSettings* Settings)
	{
		if (!Settings)
		{
			return true;
		}

		if (Settings->bIgnoreEngineAssets && PackageName.StartsWith(TEXT("/Engine/")))
		{
			return true;
		}

		if (Settings->bIgnorePluginAssets && !PackageName.StartsWith(TEXT("/Game/")) && !PackageName.StartsWith(TEXT("/Engine/")))
		{
			return true;
		}

		for (const FDirectoryPath& IgnoredPath : Settings->IgnoredPaths)
		{
			const FString NormalizedIgnoredPath = NormalizeContentDirectoryPath(IgnoredPath.Path);
			if (!NormalizedIgnoredPath.IsEmpty() && PackageName.StartsWith(NormalizedIgnoredPath))
			{
				return true;
			}
		}

		return false;
	}

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

		FString RelativeContentPath;
		if (FPaths::MakePathRelativeTo(FullPath, *FPaths::ProjectContentDir()))
		{
			RelativeContentPath = FullPath;
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

	FString GetCurrentLevelPackageName(UWorld* World)
	{
		if (!World || !World->PersistentLevel)
		{
			return FString();
		}

		return World->PersistentLevel->GetOutermost()->GetName();
	}

	FSoftObjectPath GetCurrentLevelObjectPath(UWorld* World)
	{
		const FString LevelPackageName = GetCurrentLevelPackageName(World);
		if (LevelPackageName.IsEmpty())
		{
			return FSoftObjectPath();
		}

		return FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *LevelPackageName, *FPackageName::GetShortName(LevelPackageName)));
	}

	FString GetGeneratedAssetObjectName(const FString& LevelPackageName, const ULevelDataAssetGeneratorSettings* Settings)
	{
		const FString LevelAssetName = FPackageName::GetShortName(LevelPackageName);
		return FString::Printf(TEXT("%s%s"), *Settings->GeneratedAssetPrefix, *LevelAssetName);
	}

	FString GetGeneratedAssetPackageName(const FString& LevelPackageName, const ULevelDataAssetGeneratorSettings* Settings)
	{
		return FString::Printf(
			TEXT("%s/%s"),
			*NormalizeContentDirectoryPath(Settings->OutputFolder.Path),
			*GetGeneratedAssetObjectName(LevelPackageName, Settings));
	}

	FString GetGeneratedAssetObjectPath(const FString& LevelPackageName, const ULevelDataAssetGeneratorSettings* Settings)
	{
		const FString PackageName = GetGeneratedAssetPackageName(LevelPackageName, Settings);
		const FString ObjectName = GetGeneratedAssetObjectName(LevelPackageName, Settings);
		return FString::Printf(TEXT("%s.%s"), *PackageName, *ObjectName);
	}

	FString GetInventoryPreloadAssetPackageName(const ULevelDataAssetGeneratorSettings* Settings)
	{
		return FString::Printf(
			TEXT("%s/%s"),
			*NormalizeContentDirectoryPath(Settings->InventoryPreloadOutputFolder.Path),
			*Settings->InventoryPreloadAssetName);
	}

	FString GetInventoryPreloadAssetObjectPath(const ULevelDataAssetGeneratorSettings* Settings)
	{
		const FString PackageName = GetInventoryPreloadAssetPackageName(Settings);
		return FString::Printf(TEXT("%s.%s"), *PackageName, *Settings->InventoryPreloadAssetName);
	}

	void SyncBrowserToObjectPath(const FSoftObjectPath& ObjectPath)
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		UObject* LoadedObject = ObjectPath.ResolveObject();
		if (!LoadedObject)
		{
			LoadedObject = ObjectPath.TryLoad();
		}

		if (LoadedObject)
		{
			TArray<FAssetData> Assets;
			Assets.Add(FAssetData(LoadedObject));
			ContentBrowserModule.Get().SyncBrowserToAssets(Assets);
		}
	}

	FString GetSourceTypeLabel(ELevelPreloadEntrySourceType SourceType)
	{
		switch (SourceType)
		{
		case ELevelPreloadEntrySourceType::Environment:
			return TEXT("Environment");
		case ELevelPreloadEntrySourceType::InventoryItem:
			return TEXT("InventoryItem");
		case ELevelPreloadEntrySourceType::RuntimeFeature:
			return TEXT("RuntimeFeature");
		default:
			return TEXT("Unknown");
		}
	}

	const FLevelDataAssetGeneratorPresetDefinition* FindPresetByName(const ULevelDataAssetGeneratorSettings* Settings, FName PresetName)
	{
		if (!Settings)
		{
			return nullptr;
		}

		for (const FLevelDataAssetGeneratorPresetDefinition& Preset : Settings->Presets)
		{
			if (Preset.PresetName == PresetName)
			{
				return &Preset;
			}
		}

		return nullptr;
	}

	const FLevelDataAssetGeneratorPresetDefinition* ResolvePresetForLevel(const FString& LevelPackageName, const ULevelDataAssetGeneratorSettings* Settings)
	{
		if (!Settings)
		{
			return nullptr;
		}

		const FName LevelId(*FPackageName::GetShortName(LevelPackageName));
		for (const FLevelDataAssetGeneratorPresetDefinition& Preset : Settings->Presets)
		{
			if (Preset.LevelIds.Contains(LevelId))
			{
				return &Preset;
			}
		}

		return FindPresetByName(Settings, Settings->DefaultPresetName);
	}

	const FLevelDataAssetGeneratorFeatureDefinition* FindFeatureByName(const ULevelDataAssetGeneratorSettings* Settings, FName FeatureName)
	{
		if (!Settings)
		{
			return nullptr;
		}

		for (const FLevelDataAssetGeneratorFeatureDefinition& Feature : Settings->Features)
		{
			if (Feature.FeatureName == FeatureName)
			{
				return &Feature;
			}
		}

		return nullptr;
	}

	void AddOrMergeEntry(FScanResult& Result, const TSharedPtr<FLevelPreloadEntry>& NewEntry)
	{
		if (!NewEntry.IsValid() || !NewEntry->AssetPath.IsValid())
		{
			return;
		}

		for (const TSharedPtr<FLevelPreloadEntry>& ExistingEntry : Result.Entries)
		{
			if (ExistingEntry.IsValid() &&
				ExistingEntry->AssetPath == NewEntry->AssetPath &&
				ExistingEntry->SourceType == NewEntry->SourceType)
			{
				if (!NewEntry->SourceDescription.IsEmpty() && !ExistingEntry->SourceDescription.Contains(NewEntry->SourceDescription))
				{
					ExistingEntry->SourceDescription += TEXT("; ");
					ExistingEntry->SourceDescription += NewEntry->SourceDescription;
				}
				return;
			}
		}

		Result.Entries.Add(NewEntry);
	}

	bool IsRuntimeAssetAllowed(const FAssetData& AssetData)
	{
		static const TSet<FName> BlockedClassNames =
		{
			TEXT("World"),
			TEXT("TextureRenderTarget2D"),
			TEXT("EditorUtilityWidgetBlueprint"),
			TEXT("EditorUtilityBlueprint")
		};

		if (BlockedClassNames.Contains(AssetData.AssetClassPath.GetAssetName()))
		{
			return false;
		}

		const FString PackageName = AssetData.PackageName.ToString();
		return !PackageName.Contains(TEXT("/MakeIcons/Tool/")) &&
			!PackageName.Contains(TEXT("/Developers/")) &&
			!PackageName.Contains(TEXT("/Collections/")) &&
			!PackageName.Contains(TEXT("/Editor/"));
	}

	void AddRuntimeFeatureEntry(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings, const FAssetData& AssetData, const FString& SourceDescription)
	{
		if (!Settings || !AssetData.IsValid() || !IsRuntimeAssetAllowed(AssetData))
		{
			return;
		}

		const FString PackageName = AssetData.PackageName.ToString();
		if (IsPathIgnored(PackageName, Settings))
		{
			return;
		}

		TSharedPtr<FLevelPreloadEntry> Entry = MakeShared<FLevelPreloadEntry>();
		Entry->AssetPath = AssetData.ToSoftObjectPath();
		Entry->AssetClassPath = AssetData.AssetClassPath;
		Entry->SourceDescription = SourceDescription;
		Entry->SourceType = ELevelPreloadEntrySourceType::RuntimeFeature;
		AddOrMergeEntry(Result, Entry);
	}

	void CollectRuntimeAssetWithDependencies(
		FScanResult& Result,
		const ULevelDataAssetGeneratorSettings* Settings,
		IAssetRegistry& AssetRegistry,
		const FAssetData& RootAsset,
		const FString& FeatureName,
		TSet<FName>& VisitedPackageNames)
	{
		if (!Settings || !RootAsset.IsValid() || VisitedPackageNames.Contains(RootAsset.PackageName))
		{
			return;
		}

		VisitedPackageNames.Add(RootAsset.PackageName);
		AddRuntimeFeatureEntry(Result, Settings, RootAsset, FeatureName);

		if (!Settings->bExpandRuntimeDependencies)
		{
			return;
		}

		TArray<FName> DependencyPackageNames;
		AssetRegistry.GetDependencies(
			RootAsset.PackageName,
			DependencyPackageNames,
			UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard | UE::AssetRegistry::EDependencyQuery::Soft);

		for (const FName DependencyPackageName : DependencyPackageNames)
		{
			const FString DependencyPackageNameString = DependencyPackageName.ToString();
			if (IsPathIgnored(DependencyPackageNameString, Settings))
			{
				continue;
			}

			TArray<FAssetData> DependencyAssets;
			AssetRegistry.GetAssetsByPackageName(DependencyPackageName, DependencyAssets);
			for (const FAssetData& DependencyAsset : DependencyAssets)
			{
				CollectRuntimeAssetWithDependencies(Result, Settings, AssetRegistry, DependencyAsset, FeatureName, VisitedPackageNames);
			}
		}
	}

	void CollectRuntimeAssetsFromDirectory(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings, const FDirectoryPath& Directory, const FString& FeatureName)
	{
		if (!Settings)
		{
			return;
		}

		const FString DirectoryPath = NormalizeContentDirectoryPath(Directory.Path);
		if (DirectoryPath.IsEmpty())
		{
			return;
		}

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		FARFilter Filter;
		Filter.PackagePaths.Add(FName(*DirectoryPath));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(Filter, FoundAssets);

		TSet<FName> VisitedPackageNames;
		for (const FAssetData& AssetData : FoundAssets)
		{
			CollectRuntimeAssetWithDependencies(Result, Settings, AssetRegistry, AssetData, FeatureName, VisitedPackageNames);
		}
	}

	void CollectRuntimeAssetsFromDirectories(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings, const TArray<FDirectoryPath>& Directories, const FString& FeatureName)
	{
		for (const FDirectoryPath& Directory : Directories)
		{
			CollectRuntimeAssetsFromDirectory(Result, Settings, Directory, FeatureName);
		}
	}

	void CollectRuntimeAssetsFromSoftObjects(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings, const TArray<TSoftObjectPtr<UObject>>& Assets, const FString& FeatureName)
	{
		if (!Settings)
		{
			return;
		}

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TSet<FName> VisitedPackageNames;
		for (const TSoftObjectPtr<UObject>& Asset : Assets)
		{
			const FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();
			if (!AssetPath.IsValid())
			{
				continue;
			}

			FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(AssetPath);
			if (AssetData.IsValid())
			{
				CollectRuntimeAssetWithDependencies(Result, Settings, AssetRegistry, AssetData, FeatureName, VisitedPackageNames);
			}
		}
	}

	void AddStaticMeshEntry(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings, UStaticMeshComponent* Component, AActor* Owner)
	{
		if (!Settings || !Component || !Owner)
		{
			return;
		}

		const bool bIsHISM = Component->IsA<UHierarchicalInstancedStaticMeshComponent>();
		const bool bIsISM = Component->IsA<UInstancedStaticMeshComponent>() && !bIsHISM;
		const bool bIsPlainStaticMesh = !bIsISM && !bIsHISM;

		if ((bIsHISM && !Settings->bIncludeHierarchicalInstancedStaticMeshComponent) ||
			(bIsISM && !Settings->bIncludeInstancedStaticMeshComponent) ||
			(bIsPlainStaticMesh && !Settings->bIncludeStaticMeshComponent))
		{
			return;
		}

		UStaticMesh* StaticMesh = Component->GetStaticMesh();
		if (!StaticMesh)
		{
			return;
		}

		const FSoftObjectPath AssetPath(StaticMesh);
		const FString PackageName = FPackageName::ObjectPathToPackageName(AssetPath.ToString());
		if (IsPathIgnored(PackageName, Settings))
		{
			return;
		}

		TSharedPtr<FLevelPreloadEntry> Entry = MakeShared<FLevelPreloadEntry>();
		Entry->AssetPath = AssetPath;
		Entry->AssetClassPath = UStaticMesh::StaticClass()->GetClassPathName();
		Entry->SourceDescription = FString::Printf(TEXT("%s.%s"), *Owner->GetActorLabel(), *Component->GetName());
		Entry->SourceType = ELevelPreloadEntrySourceType::Environment;
		AddOrMergeEntry(Result, Entry);
	}

	bool IsAssetClassMatched(const FAssetData& AssetData, UClass* TargetClass)
	{
		if (!TargetClass)
		{
			return true;
		}

		if (AssetData.AssetClassPath == TargetClass->GetClassPathName())
		{
			return true;
		}

		UClass* AssetClass = AssetData.GetClass();
		return AssetClass && AssetClass->IsChildOf(TargetClass);
	}

	void CollectInventoryItemAssets(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings)
	{
		if (!Settings)
		{
			return;
		}

		const FString ItemDataDirectory = NormalizeContentDirectoryPath(Settings->InventoryItemDataDirectory.Path);
		if (ItemDataDirectory.IsEmpty())
		{
			return;
		}

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		FARFilter Filter;
		Filter.PackagePaths.Add(FName(*ItemDataDirectory));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(Filter, FoundAssets);

		UClass* ItemDataAssetClass = Settings->InventoryItemDataAssetClass.LoadSynchronous();
		for (const FAssetData& AssetData : FoundAssets)
		{
			if (!IsAssetClassMatched(AssetData, ItemDataAssetClass))
			{
				continue;
			}

			TSharedPtr<FLevelPreloadEntry> Entry = MakeShared<FLevelPreloadEntry>();
			Entry->AssetPath = AssetData.ToSoftObjectPath();
			Entry->AssetClassPath = AssetData.AssetClassPath;
			Entry->SourceDescription = ItemDataDirectory;
			Entry->SourceType = ELevelPreloadEntrySourceType::InventoryItem;
			AddOrMergeEntry(Result, Entry);
		}
	}

	void CollectRuntimeFeatureAssets(FScanResult& Result, const ULevelDataAssetGeneratorSettings* Settings)
	{
		if (!Settings)
		{
			return;
		}

		const FLevelDataAssetGeneratorPresetDefinition* Preset = FindPresetByName(Settings, Result.PresetName);
		if (!Preset)
		{
			return;
		}

		for (const FName FeatureName : Preset->FeatureNames)
		{
			const FLevelDataAssetGeneratorFeatureDefinition* Feature = FindFeatureByName(Settings, FeatureName);
			if (!Feature)
			{
				UE_LOG(LogTemp, Warning, TEXT("LevelDataAssetGenerator: Feature '%s' was referenced by preset '%s' but was not found."),
					*FeatureName.ToString(),
					*Preset->PresetName.ToString());
				continue;
			}

			const FString FeatureNameString = Feature->FeatureName.ToString();
			CollectRuntimeAssetsFromDirectories(Result, Settings, Feature->RootDirectories, FeatureNameString);
			CollectRuntimeAssetsFromSoftObjects(Result, Settings, Feature->DirectAssets, FeatureNameString);
		}
	}

	FText GetPresetText(FName PresetName)
	{
		return FText::FromName(PresetName);
	}

	FScanResult ScanCurrentLevel()
	{
		FScanResult Result;
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();

		Result.LevelPackageName = GetCurrentLevelPackageName(World);
		Result.LevelObjectPath = GetCurrentLevelObjectPath(World);
		const FLevelDataAssetGeneratorPresetDefinition* Preset = ResolvePresetForLevel(Result.LevelPackageName, Settings);
		Result.PresetName = Preset ? Preset->PresetName : NAME_None;

		if (!World || !Settings)
		{
			return Result;
		}

		if (Settings->bIncludePlacedEnvironmentAssets)
		{
			for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
			{
				AActor* Actor = *ActorIterator;
				if (!IsValid(Actor))
				{
					continue;
				}

				TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents(Actor);
				for (UStaticMeshComponent* Component : StaticMeshComponents)
				{
					AddStaticMeshEntry(Result, Settings, Component, Actor);
				}
			}
		}

		CollectRuntimeFeatureAssets(Result, Settings);

		Result.Entries.Sort([](const TSharedPtr<FLevelPreloadEntry>& Left, const TSharedPtr<FLevelPreloadEntry>& Right)
		{
			if (!Left.IsValid() || !Right.IsValid())
			{
				return Left.IsValid();
			}

			if (Left->SourceType != Right->SourceType)
			{
				return static_cast<uint8>(Left->SourceType) < static_cast<uint8>(Right->SourceType);
			}

			return Left->AssetPath.ToString() < Right->AssetPath.ToString();
		});

		return Result;
	}

	FScanResult ScanInventoryPreload()
	{
		FScanResult Result;
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!Settings)
		{
			return Result;
		}

		Result.LevelPackageName = GetInventoryPreloadAssetPackageName(Settings);
		CollectInventoryItemAssets(Result, Settings);

		Result.Entries.Sort([](const TSharedPtr<FLevelPreloadEntry>& Left, const TSharedPtr<FLevelPreloadEntry>& Right)
		{
			if (!Left.IsValid() || !Right.IsValid())
			{
				return Left.IsValid();
			}

			return Left->AssetPath.ToString() < Right->AssetPath.ToString();
		});

		return Result;
	}

	bool SetSoftObjectArrayProperty(UObject* DataAsset, FName PropertyName, const TArray<FSoftObjectPath>& AssetPaths)
	{
		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(DataAsset->GetClass(), PropertyName);
		FSoftObjectProperty* InnerSoftObjectProperty = ArrayProperty ? CastField<FSoftObjectProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !InnerSoftObjectProperty)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(
					LOCTEXT("MissingSoftObjectArrayProperty", "Missing soft object array property: {0}"),
					FText::FromName(PropertyName)));
			return false;
		}

		void* ArrayValue = ArrayProperty->ContainerPtrToValuePtr<void>(DataAsset);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayValue);
		ArrayHelper.EmptyAndAddValues(AssetPaths.Num());

		for (int32 Index = 0; Index < AssetPaths.Num(); ++Index)
		{
			InnerSoftObjectProperty->SetPropertyValue(ArrayHelper.GetRawPtr(Index), FSoftObjectPtr(AssetPaths[Index]));
		}

		return true;
	}

	bool SetSoftObjectProperty(UObject* DataAsset, FName PropertyName, const FSoftObjectPath& AssetPath)
	{
		FSoftObjectProperty* SoftObjectProperty = FindFProperty<FSoftObjectProperty>(DataAsset->GetClass(), PropertyName);
		if (!SoftObjectProperty)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(
					LOCTEXT("MissingSoftObjectProperty", "Missing soft object property: {0}"),
					FText::FromName(PropertyName)));
			return false;
		}

		SoftObjectProperty->SetPropertyValue_InContainer(DataAsset, FSoftObjectPtr(AssetPath));
		return true;
	}

	bool SetBoolProperty(UObject* DataAsset, FName PropertyName, bool bValue)
	{
		FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(DataAsset->GetClass(), PropertyName);
		if (!BoolProperty)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(
					LOCTEXT("MissingBoolProperty", "Missing bool property: {0}"),
					FText::FromName(PropertyName)));
			return false;
		}

		BoolProperty->SetPropertyValue_InContainer(DataAsset, bValue);
		return true;
	}

	bool SetLevelDataAssetProperties(
		UObject* DataAsset,
		const FScanResult& Result,
		const TArray<FSoftObjectPath>& IncludedEnvironmentAssets,
		const TArray<FSoftObjectPath>& IncludedRuntimeAssets)
	{
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!DataAsset || !Settings)
		{
			return false;
		}

		if (FNameProperty* LevelIdProperty = FindFProperty<FNameProperty>(DataAsset->GetClass(), Settings->LevelIdPropertyName))
		{
			LevelIdProperty->SetPropertyValue_InContainer(DataAsset, FName(*FPackageName::GetShortName(Result.LevelPackageName)));
		}

		if (FSoftObjectProperty* LevelProperty = FindFProperty<FSoftObjectProperty>(DataAsset->GetClass(), Settings->LevelPropertyName))
		{
			LevelProperty->SetPropertyValue_InContainer(DataAsset, FSoftObjectPtr(Result.LevelObjectPath));
		}

		if (!SetSoftObjectArrayProperty(DataAsset, Settings->GeneratedEnvironmentAssetsPropertyName, IncludedEnvironmentAssets))
		{
			return false;
		}

		if (!SetSoftObjectArrayProperty(DataAsset, Settings->GeneratedRuntimeAssetsPropertyName, IncludedRuntimeAssets))
		{
			return false;
		}

		const FLevelDataAssetGeneratorPresetDefinition* Preset = FindPresetByName(Settings, Result.PresetName);
		const bool bAssignInventoryPreloadDataAsset = Settings->bAssignInventoryPreloadToLevelDataAssets &&
			Preset &&
			Preset->bAssignInventoryPreloadDataAsset;

		if (!SetBoolProperty(DataAsset, Settings->bUseInventoryPreloadDataAssetPropertyName, bAssignInventoryPreloadDataAsset))
		{
			return false;
		}

		return SetSoftObjectProperty(
			DataAsset,
			Settings->InventoryPreloadDataAssetPropertyName,
			bAssignInventoryPreloadDataAsset
				? FSoftObjectPath(GetInventoryPreloadAssetObjectPath(Settings))
				: FSoftObjectPath());
	}

	bool SetInventoryPreloadDataAssetProperties(
		UObject* DataAsset,
		const TArray<FSoftObjectPath>& IncludedInventoryItemAssets)
	{
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!DataAsset || !Settings)
		{
			return false;
		}

		if (!SetBoolProperty(DataAsset, Settings->bIncludeAllPrimaryItemAssetsPropertyName, false))
		{
			return false;
		}

		return SetSoftObjectArrayProperty(DataAsset, Settings->InventoryItemAssetsPropertyName, IncludedInventoryItemAssets);
	}

	UObject* LoadOrCreateDataAsset(
		const FString& ObjectPath,
		const FString& PackageName,
		const FString& ObjectName,
		UClass* DataAssetClass)
	{
		if (ObjectPath.IsEmpty() || PackageName.IsEmpty() || ObjectName.IsEmpty())
		{
			return nullptr;
		}

		if (UObject* ExistingAsset = LoadObject<UObject>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		if (!DataAssetClass || DataAssetClass->HasAnyClassFlags(CLASS_Abstract))
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				LOCTEXT("InvalidGeneratedDataAssetClass", "GeneratedDataAssetClass must be a non-abstract DataAsset class."));
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		UObject* NewAsset = NewObject<UObject>(Package, DataAssetClass, *ObjectName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(NewAsset);
		Package->MarkPackageDirty();
		return NewAsset;
	}

	UObject* LoadOrCreateGeneratedDataAsset(const FScanResult& Result)
	{
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!Settings || Result.LevelPackageName.IsEmpty())
		{
			return nullptr;
		}

		return LoadOrCreateDataAsset(
			GetGeneratedAssetObjectPath(Result.LevelPackageName, Settings),
			GetGeneratedAssetPackageName(Result.LevelPackageName, Settings),
			GetGeneratedAssetObjectName(Result.LevelPackageName, Settings),
			Settings->GeneratedDataAssetClass.LoadSynchronous());
	}

	UObject* LoadOrCreateInventoryPreloadDataAsset()
	{
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!Settings)
		{
			return nullptr;
		}

		return LoadOrCreateDataAsset(
			GetInventoryPreloadAssetObjectPath(Settings),
			GetInventoryPreloadAssetPackageName(Settings),
			Settings->InventoryPreloadAssetName,
			Settings->InventoryPreloadDataAssetClass.LoadSynchronous());
	}

	bool GenerateDataAssetFromEntries(const FScanResult& Result)
	{
		UObject* DataAsset = LoadOrCreateGeneratedDataAsset(Result);
		if (!DataAsset)
		{
			return false;
		}

		TArray<FSoftObjectPath> IncludedEnvironmentAssets;
		TArray<FSoftObjectPath> IncludedRuntimeAssets;
		TSet<FString> AddedEnvironmentPaths;
		TSet<FString> AddedRuntimePaths;
		for (const TSharedPtr<FLevelPreloadEntry>& Entry : Result.Entries)
		{
			if (!Entry.IsValid() || !Entry->bIncluded)
			{
				continue;
			}

			const FString AssetPathString = Entry->AssetPath.ToString();
			if (Entry->SourceType == ELevelPreloadEntrySourceType::Environment && !AddedEnvironmentPaths.Contains(AssetPathString))
			{
				IncludedEnvironmentAssets.Add(Entry->AssetPath);
				AddedEnvironmentPaths.Add(AssetPathString);
			}
			else if (Entry->SourceType == ELevelPreloadEntrySourceType::RuntimeFeature && !AddedRuntimePaths.Contains(AssetPathString))
			{
				IncludedRuntimeAssets.Add(Entry->AssetPath);
				AddedRuntimePaths.Add(AssetPathString);
			}
		}

		DataAsset->Modify();
		if (!SetLevelDataAssetProperties(DataAsset, Result, IncludedEnvironmentAssets, IncludedRuntimeAssets))
		{
			return false;
		}

		DataAsset->MarkPackageDirty();

		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(DataAsset->GetOutermost());
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
		SyncBrowserToObjectPath(FSoftObjectPath(DataAsset));
		return true;
	}

	bool GenerateInventoryPreloadDataAssetFromEntries(const FScanResult& Result)
	{
		UObject* DataAsset = LoadOrCreateInventoryPreloadDataAsset();
		if (!DataAsset)
		{
			return false;
		}

		TArray<FSoftObjectPath> IncludedInventoryItemAssets;
		TSet<FString> AddedInventoryItemPaths;
		for (const TSharedPtr<FLevelPreloadEntry>& Entry : Result.Entries)
		{
			if (!Entry.IsValid() || !Entry->bIncluded || Entry->SourceType != ELevelPreloadEntrySourceType::InventoryItem)
			{
				continue;
			}

			const FString AssetPathString = Entry->AssetPath.ToString();
			if (!AddedInventoryItemPaths.Contains(AssetPathString))
			{
				IncludedInventoryItemAssets.Add(Entry->AssetPath);
				AddedInventoryItemPaths.Add(AssetPathString);
			}
		}

		DataAsset->Modify();
		if (!SetInventoryPreloadDataAssetProperties(DataAsset, IncludedInventoryItemAssets))
		{
			return false;
		}

		DataAsset->MarkPackageDirty();

		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(DataAsset->GetOutermost());
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
		SyncBrowserToObjectPath(FSoftObjectPath(DataAsset));
		return true;
	}
}

class SLevelDataAssetGeneratorEntryRow : public SMultiColumnTableRow<TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry>>
{
public:
	SLATE_BEGIN_ARGS(SLevelDataAssetGeneratorEntryRow) {}
		SLATE_ARGUMENT(TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry>, Entry)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Entry = InArgs._Entry;
		SMultiColumnTableRow<TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry>>::Construct(
			FSuperRowType::FArguments(),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Entry.IsValid())
		{
			return SNew(STextBlock).Text(FText::GetEmpty());
		}

		if (ColumnName == TEXT("Include"))
		{
			return SNew(SCheckBox)
				.IsChecked_Lambda([Entry = Entry]()
				{
					return Entry->bIncluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([Entry = Entry](ECheckBoxState NewState)
				{
					Entry->bIncluded = NewState == ECheckBoxState::Checked;
				});
		}

		if (ColumnName == TEXT("Asset"))
		{
			return SNew(STextBlock).Text(FText::FromString(Entry->AssetPath.GetAssetName()));
		}

		if (ColumnName == TEXT("Class"))
		{
			return SNew(STextBlock).Text(FText::FromString(Entry->AssetClassPath.GetAssetName().ToString()));
		}

		if (ColumnName == TEXT("Source"))
		{
			return SNew(STextBlock).Text(FText::FromString(FString::Printf(
				TEXT("%s: %s"),
				*LevelDataAssetGeneratorEditor::GetSourceTypeLabel(Entry->SourceType),
				*Entry->SourceDescription)));
		}

		if (ColumnName == TEXT("Sync"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("SyncButton", "Sync"))
				.OnClicked_Lambda([Entry = Entry]()
				{
					LevelDataAssetGeneratorEditor::SyncBrowserToObjectPath(Entry->AssetPath);
					return FReply::Handled();
				});
		}

		return SNew(STextBlock).Text(FText::GetEmpty());
	}

private:
	TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry> Entry;
};

class FLevelDataAssetGeneratorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		RegisterSettings();
		RegisterConsoleCommands();

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}

		UnregisterSettings();
		GenerateCurrentLevelConsoleCommand.Reset();
		GenerateInventoryPreloadConsoleCommand.Reset();
	}

private:
	void RegisterConsoleCommands()
	{
		GenerateCurrentLevelConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("FT.LevelDataAssetGenerator.GenerateCurrentLevelPreloadData"),
			TEXT("Scan the currently opened editor level and generate its level preload DataAsset."),
			FConsoleCommandDelegate::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::GenerateCurrentLevelPreloadDataAsset));

		GenerateInventoryPreloadConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("FT.LevelDataAssetGenerator.GenerateInventoryItemPreloadData"),
			TEXT("Scan configured inventory item DataAssets and generate the shared inventory preload DataAsset."),
			FConsoleCommandDelegate::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::GenerateInventoryItemPreloadDataAsset));
	}

	void RegisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
		{
			SettingsModule->RegisterSettings(
				TEXT("Project"),
				TEXT("Plugins"),
				TEXT("LevelDataAssetGenerator"),
				LOCTEXT("LevelDataAssetGeneratorSettingsName", "Level Data Asset Generator"),
				LOCTEXT("LevelDataAssetGeneratorSettingsDescription", "Configure level preload DataAsset generation."),
				GetMutableDefault<ULevelDataAssetGeneratorSettings>());
		}
	}

	void UnregisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
		{
			SettingsModule->UnregisterSettings(TEXT("Project"), TEXT("Plugins"), TEXT("LevelDataAssetGenerator"));
		}
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("LevelDataAssetGenerator"));

		Section.AddMenuEntry(
			TEXT("LevelDataAssetGeneratorGenerateFromCurrentLevel"),
			LOCTEXT("GenerateFromCurrentLevel", "Generate Level Preload Data Asset"),
			LOCTEXT("GenerateFromCurrentLevelTooltip", "Collect runtime feature assets for the current level and update the level preload DataAsset."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::OpenGenerateWindow)));

		Section.AddMenuEntry(
			TEXT("LevelDataAssetGeneratorGenerateInventoryItemPreload"),
			LOCTEXT("GenerateInventoryItemPreload", "Generate Inventory Item Preload Data Asset"),
			LOCTEXT("GenerateInventoryItemPreloadTooltip", "Scan configured inventory item DataAssets and update the shared inventory preload DataAsset."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::OpenGenerateInventoryWindow)));
	}

	void GenerateCurrentLevelPreloadDataAsset()
	{
		using namespace LevelDataAssetGeneratorEditor;

		const FScanResult Result = ScanCurrentLevel();
		if (Result.LevelPackageName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("LevelDataAssetGenerator: No editor level is open."));
			return;
		}

		if (GenerateDataAssetFromEntries(Result))
		{
			UE_LOG(LogTemp, Log, TEXT("LevelDataAssetGenerator: Generated preload DataAsset for %s with %d entries."),
				*Result.LevelPackageName,
				Result.Entries.Num());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LevelDataAssetGenerator: Failed to generate preload DataAsset for %s."),
				*Result.LevelPackageName);
		}
	}

	void GenerateInventoryItemPreloadDataAsset()
	{
		using namespace LevelDataAssetGeneratorEditor;

		const FScanResult Result = ScanInventoryPreload();
		if (GenerateInventoryPreloadDataAssetFromEntries(Result))
		{
			UE_LOG(LogTemp, Log, TEXT("LevelDataAssetGenerator: Generated inventory preload DataAsset with %d entries."),
				Result.Entries.Num());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LevelDataAssetGenerator: Failed to generate inventory preload DataAsset."));
		}
	}

	void OpenGenerateWindow()
	{
		using namespace LevelDataAssetGeneratorEditor;

		TSharedRef<FScanResult> Result = MakeShared<FScanResult>(ScanCurrentLevel());
		if (Result->LevelPackageName.IsEmpty())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoLevelOpen", "No editor level is open."));
			return;
		}

		TSharedRef<SListView<TSharedPtr<FLevelPreloadEntry>>> ListView =
			SNew(SListView<TSharedPtr<FLevelPreloadEntry>>)
			.ListItemsSource(&Result->Entries)
			.OnGenerateRow_Lambda([](TSharedPtr<FLevelPreloadEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return SNew(SLevelDataAssetGeneratorEntryRow, OwnerTable).Entry(Entry);
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(TEXT("Include")).DefaultLabel(LOCTEXT("IncludeColumn", "Include")).FixedWidth(70.0f)
				+ SHeaderRow::Column(TEXT("Asset")).DefaultLabel(LOCTEXT("AssetColumn", "Asset")).FillWidth(0.32f)
				+ SHeaderRow::Column(TEXT("Class")).DefaultLabel(LOCTEXT("ClassColumn", "Class")).FillWidth(0.14f)
				+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("SourceColumn", "Source")).FillWidth(0.45f)
				+ SHeaderRow::Column(TEXT("Sync")).DefaultLabel(LOCTEXT("SyncColumn", "Sync")).FixedWidth(70.0f)
			);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(FText::Format(
				LOCTEXT("GenerateWindowTitle", "Level Data Asset Generator - {0}"),
				FText::FromString(Result->LevelPackageName)))
			.ClientSize(FVector2D(1000.0f, 580.0f));

		Window->SetContent(
			SNew(SBorder)
			.Padding(8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([Result]()
					{
						return FText::Format(
							LOCTEXT("ScanSummary", "Level: {0} / Preset: {1} / Runtime Assets: {2} / Environment Assets: {3}"),
							FText::FromString(Result->LevelPackageName),
							LevelDataAssetGeneratorEditor::GetPresetText(Result->PresetName),
							FText::AsNumber(Result->Entries.FilterByPredicate([](const TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry>& Entry)
							{
								return Entry.IsValid() && Entry->SourceType == LevelDataAssetGeneratorEditor::ELevelPreloadEntrySourceType::RuntimeFeature;
							}).Num()),
							FText::AsNumber(Result->Entries.FilterByPredicate([](const TSharedPtr<LevelDataAssetGeneratorEditor::FLevelPreloadEntry>& Entry)
							{
								return Entry.IsValid() && Entry->SourceType == LevelDataAssetGeneratorEditor::ELevelPreloadEntrySourceType::Environment;
							}).Num()));
					})
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					ListView
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("GenerateButton", "Generate Level Preload Data Asset"))
					.OnClicked_Lambda([Result, Window]()
					{
						if (LevelDataAssetGeneratorEditor::GenerateDataAssetFromEntries(*Result))
						{
							Window->RequestDestroyWindow();
						}

						return FReply::Handled();
					})
				]
			]);

		FSlateApplication::Get().AddWindow(Window);
	}

	void OpenGenerateInventoryWindow()
	{
		using namespace LevelDataAssetGeneratorEditor;

		TSharedRef<FScanResult> Result = MakeShared<FScanResult>(ScanInventoryPreload());

		TSharedRef<SListView<TSharedPtr<FLevelPreloadEntry>>> ListView =
			SNew(SListView<TSharedPtr<FLevelPreloadEntry>>)
			.ListItemsSource(&Result->Entries)
			.OnGenerateRow_Lambda([](TSharedPtr<FLevelPreloadEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return SNew(SLevelDataAssetGeneratorEntryRow, OwnerTable).Entry(Entry);
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(TEXT("Include")).DefaultLabel(LOCTEXT("IncludeColumn", "Include")).FixedWidth(70.0f)
				+ SHeaderRow::Column(TEXT("Asset")).DefaultLabel(LOCTEXT("AssetColumn", "Asset")).FillWidth(0.32f)
				+ SHeaderRow::Column(TEXT("Class")).DefaultLabel(LOCTEXT("ClassColumn", "Class")).FillWidth(0.14f)
				+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("SourceColumn", "Source")).FillWidth(0.45f)
				+ SHeaderRow::Column(TEXT("Sync")).DefaultLabel(LOCTEXT("SyncColumn", "Sync")).FixedWidth(70.0f)
			);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("GenerateInventoryWindowTitle", "Level Data Asset Generator - Inventory Item Preload"))
			.ClientSize(FVector2D(1000.0f, 580.0f));

		Window->SetContent(
			SNew(SBorder)
			.Padding(8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([Result]()
					{
						return FText::Format(
							LOCTEXT("InventoryScanSummary", "Inventory Preload DataAsset: {0} / Item DataAssets: {1}"),
							FText::FromString(Result->LevelPackageName),
							FText::AsNumber(Result->Entries.Num()));
					})
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					ListView
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("GenerateInventoryButton", "Generate Inventory Item Preload Data Asset"))
					.OnClicked_Lambda([Result, Window]()
					{
						if (LevelDataAssetGeneratorEditor::GenerateInventoryPreloadDataAssetFromEntries(*Result))
						{
							Window->RequestDestroyWindow();
						}

						return FReply::Handled();
					})
				]
			]);

		FSlateApplication::Get().AddWindow(Window);
	}

	TUniquePtr<FAutoConsoleCommand> GenerateCurrentLevelConsoleCommand;
	TUniquePtr<FAutoConsoleCommand> GenerateInventoryPreloadConsoleCommand;
};

IMPLEMENT_MODULE(FLevelDataAssetGeneratorEditorModule, LevelDataAssetGeneratorEditor)

#undef LOCTEXT_NAMESPACE
