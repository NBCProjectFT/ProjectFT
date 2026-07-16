#include "FTCheatManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FTLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Modules/ModuleManager.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
#include "ProjectFT/Item/FTItemPoolSubsystem.h"
#include "UObject/UObjectIterator.h"

namespace
{
	const FName FTItemDataDirectory(TEXT("/Game/Blueprints/Items/Data"));

	struct FFTLoadedAssetMemoryInfo
	{
		FString Path;
		FString ClassName;
		SIZE_T SizeBytes = 0;
	};

	FName MakeItemId(const FString& ItemIdText)
	{
		return FName(*ItemIdText.TrimStartAndEnd());
	}

	FString FormatAssetSize(SIZE_T SizeBytes)
	{
		const double SizeMB = static_cast<double>(SizeBytes) / 1024.0 / 1024.0;
		return FString::Printf(TEXT("%.3f MB"), SizeMB);
	}
}

void UFTCheatManager::GiveItem(const FString& ItemId, int32 Quantity)
{
	APlayerController* PlayerController = GetOuterAPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UFTInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UFTInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		UE_LOG(LogFTItem, Warning, TEXT("GiveItem failed: player inventory was not found."));
		return;
	}

	const FName ItemName = MakeItemId(ItemId);
	Quantity = FMath::Max(1, Quantity);
	if (ItemName.IsNone())
	{
		UE_LOG(LogFTItem, Warning, TEXT("GiveItem failed: ItemId is empty."));
		return;
	}

	if (!UFTItemFunctionLibrary::FindItemData(this, ItemName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("GiveItem failed: item data was not found. ItemId=%s"), *ItemName.ToString());
		return;
	}

	const bool bAdded = Inventory->AddItem(ItemName, Quantity);
	UE_LOG(LogFTItem, Warning, TEXT("Cheat GiveItem %s. ItemId=%s Quantity=%d"),
		bAdded ? TEXT("succeeded") : TEXT("failed"),
		*ItemName.ToString(),
		Quantity);
}

void UFTCheatManager::SpawnItem(const FString& ItemId, int32 Quantity)
{
	APlayerController* PlayerController = GetOuterAPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWorld* World = GetWorld();
	UFTItemPoolSubsystem* ItemPool = World ? World->GetSubsystem<UFTItemPoolSubsystem>() : nullptr;
	if (!Pawn || !ItemPool)
	{
		UE_LOG(LogFTItem, Warning, TEXT("SpawnItem failed: pawn or item pool was not found."));
		return;
	}

	const FName ItemName = MakeItemId(ItemId);
	Quantity = FMath::Max(1, Quantity);
	if (ItemName.IsNone())
	{
		UE_LOG(LogFTItem, Warning, TEXT("SpawnItem failed: ItemId is empty."));
		return;
	}

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Quantity; ++Index)
	{
		const FVector SpawnLocation = UFTItemFunctionLibrary::CalculateDropLocation(Pawn);
		const FRotator SpawnRotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
		if (ItemPool->AcquireItemActor(ItemName, SpawnLocation, SpawnRotation))
		{
			++SpawnedCount;
		}
	}

	UE_LOG(LogFTItem, Warning, TEXT("Cheat SpawnItem spawned %d / %d. ItemId=%s"),
		SpawnedCount,
		Quantity,
		*ItemName.ToString());
}

void UFTCheatManager::ShowItemList(const FString& Filter)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter AssetFilter;
	AssetFilter.PackagePaths.Add(FTItemDataDirectory);
	AssetFilter.ClassPaths.Add(UFTItemDataAsset::StaticClass()->GetClassPathName());
	AssetFilter.bRecursivePaths = true;
	AssetFilter.bRecursiveClasses = true;

	TArray<FAssetData> ItemAssetDataList;
	AssetRegistry.GetAssets(AssetFilter, ItemAssetDataList);
	ItemAssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	const FString NormalizedFilter = Filter.TrimStartAndEnd();
	int32 DisplayedCount = 0;

	UE_LOG(LogFTItem, Warning, TEXT("Item ID list begin. Assets=%d Filter='%s'"), ItemAssetDataList.Num(), *NormalizedFilter);

	for (const FAssetData& ItemAssetData : ItemAssetDataList)
	{
		UFTItemDataAsset* ItemDataAsset = Cast<UFTItemDataAsset>(ItemAssetData.GetAsset());
		if (!ItemDataAsset)
		{
			continue;
		}

		const FString ItemIdText = ItemDataAsset->ItemData.ItemId.ToString();
		const FString AssetNameText = ItemAssetData.AssetName.ToString();
		const FString PathText = ItemAssetData.GetSoftObjectPath().ToString();
		if (!NormalizedFilter.IsEmpty()
			&& !ItemIdText.Contains(NormalizedFilter, ESearchCase::IgnoreCase)
			&& !AssetNameText.Contains(NormalizedFilter, ESearchCase::IgnoreCase)
			&& !PathText.Contains(NormalizedFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		++DisplayedCount;
		UE_LOG(LogFTItem, Warning, TEXT("[%03d] %s | %s | %s"),
			DisplayedCount,
			*ItemIdText,
			*AssetNameText,
			*PathText);
	}

	UE_LOG(LogFTItem, Warning, TEXT("Item ID list end. Displayed=%d / %d"), DisplayedCount, ItemAssetDataList.Num());
}

void UFTCheatManager::FTDumpLoadedAssets(int32 MaxCount)
{
	MaxCount = FMath::Max(1, MaxCount);

	TArray<FFTLoadedAssetMemoryInfo> LoadedAssets;
	SIZE_T TotalBytes = 0;

	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject* Object = *It;
		if (!IsValid(Object) || !Object->IsAsset())
		{
			continue;
		}

		UPackage* Package = Object->GetOutermost();
		if (!Package || Package->HasAnyPackageFlags(PKG_CompiledIn))
		{
			continue;
		}

		const SIZE_T SizeBytes = Object->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
		if (SizeBytes == 0)
		{
			continue;
		}

		FFTLoadedAssetMemoryInfo Info;
		Info.Path = Object->GetPathName();
		Info.ClassName = Object->GetClass() ? Object->GetClass()->GetName() : TEXT("Unknown");
		Info.SizeBytes = SizeBytes;
		LoadedAssets.Add(MoveTemp(Info));
		TotalBytes += SizeBytes;
	}

	LoadedAssets.Sort([](const FFTLoadedAssetMemoryInfo& Left, const FFTLoadedAssetMemoryInfo& Right)
	{
		return Left.SizeBytes > Right.SizeBytes;
	});

	UE_LOG(LogFTAsset, Warning, TEXT("Loaded asset memory dump: Assets=%d Total=%s ShowingTop=%d"),
		LoadedAssets.Num(),
		*FormatAssetSize(TotalBytes),
		FMath::Min(MaxCount, LoadedAssets.Num()));

	for (int32 Index = 0; Index < LoadedAssets.Num() && Index < MaxCount; ++Index)
	{
		const FFTLoadedAssetMemoryInfo& Info = LoadedAssets[Index];
		UE_LOG(LogFTAsset, Warning, TEXT("[%03d] %s | %s | %s"),
			Index + 1,
			*FormatAssetSize(Info.SizeBytes),
			*Info.ClassName,
			*Info.Path);
	}
}
