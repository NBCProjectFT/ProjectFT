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
		InventoryItem
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
		if (!Settings || !Settings->bGenerateInventoryItemPreloadAssets)
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

	FScanResult ScanCurrentLevel()
	{
		FScanResult Result;
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();

		Result.LevelPackageName = GetCurrentLevelPackageName(World);
		Result.LevelObjectPath = GetCurrentLevelObjectPath(World);

		if (!World || !Settings)
		{
			return Result;
		}

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

		CollectInventoryItemAssets(Result, Settings);

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

	bool SetLevelDataAssetProperties(
		UObject* DataAsset,
		const FScanResult& Result,
		const TArray<FSoftObjectPath>& IncludedEnvironmentAssets,
		const TArray<FSoftObjectPath>& IncludedInventoryItemAssets)
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

		if (Settings->bGenerateInventoryItemPreloadAssets)
		{
			if (!SetSoftObjectArrayProperty(DataAsset, Settings->GeneratedInventoryItemAssetsPropertyName, IncludedInventoryItemAssets))
			{
				return false;
			}
		}

		return true;
	}

	UObject* LoadOrCreateGeneratedDataAsset(const FScanResult& Result)
	{
		const ULevelDataAssetGeneratorSettings* Settings = GetDefault<ULevelDataAssetGeneratorSettings>();
		if (!Settings || Result.LevelPackageName.IsEmpty())
		{
			return nullptr;
		}

		const FString ObjectPath = GetGeneratedAssetObjectPath(Result.LevelPackageName, Settings);
		if (UObject* ExistingAsset = LoadObject<UObject>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		UClass* DataAssetClass = Settings->GeneratedDataAssetClass.LoadSynchronous();
		if (!DataAssetClass || DataAssetClass->HasAnyClassFlags(CLASS_Abstract))
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				LOCTEXT("InvalidGeneratedDataAssetClass", "GeneratedDataAssetClass must be a non-abstract DataAsset class."));
			return nullptr;
		}

		const FString PackageName = GetGeneratedAssetPackageName(Result.LevelPackageName, Settings);
		const FString ObjectName = GetGeneratedAssetObjectName(Result.LevelPackageName, Settings);
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

	bool GenerateDataAssetFromEntries(const FScanResult& Result)
	{
		UObject* DataAsset = LoadOrCreateGeneratedDataAsset(Result);
		if (!DataAsset)
		{
			return false;
		}

		TArray<FSoftObjectPath> IncludedEnvironmentAssets;
		TArray<FSoftObjectPath> IncludedInventoryItemAssets;
		TSet<FString> AddedEnvironmentPaths;
		TSet<FString> AddedInventoryItemPaths;
		for (const TSharedPtr<FLevelPreloadEntry>& Entry : Result.Entries)
		{
			if (!Entry.IsValid() || !Entry->bIncluded)
			{
				continue;
			}

			const FString AssetPathString = Entry->AssetPath.ToString();
			if (Entry->SourceType == ELevelPreloadEntrySourceType::InventoryItem)
			{
				if (!AddedInventoryItemPaths.Contains(AssetPathString))
				{
					IncludedInventoryItemAssets.Add(Entry->AssetPath);
					AddedInventoryItemPaths.Add(AssetPathString);
				}
			}
			else if (!AddedEnvironmentPaths.Contains(AssetPathString))
			{
				IncludedEnvironmentAssets.Add(Entry->AssetPath);
				AddedEnvironmentPaths.Add(AssetPathString);
			}
		}

		DataAsset->Modify();
		if (!SetLevelDataAssetProperties(DataAsset, Result, IncludedEnvironmentAssets, IncludedInventoryItemAssets))
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
			return SNew(STextBlock).Text(FText::FromString(Entry->SourceDescription));
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
	}

private:
	void RegisterConsoleCommands()
	{
		GenerateCurrentLevelConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("FT.LevelDataAssetGenerator.GenerateCurrentLevelPreloadData"),
			TEXT("Scan the currently opened editor level and generate its level preload DataAsset."),
			FConsoleCommandDelegate::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::GenerateCurrentLevelPreloadDataAsset));
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
			LOCTEXT("GenerateFromCurrentLevelTooltip", "Scan current level environment meshes and update the level preload DataAsset."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FLevelDataAssetGeneratorEditorModule::OpenGenerateWindow)));
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
							LOCTEXT("ScanSummary", "Level: {0} / Environment Assets: {1}"),
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

	TUniquePtr<FAutoConsoleCommand> GenerateCurrentLevelConsoleCommand;
};

IMPLEMENT_MODULE(FLevelDataAssetGeneratorEditorModule, LevelDataAssetGeneratorEditor)

#undef LOCTEXT_NAMESPACE
