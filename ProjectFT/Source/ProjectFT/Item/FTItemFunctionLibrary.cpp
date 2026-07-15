#include "FTItemFunctionLibrary.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Modules/ModuleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace
{
	const FName FTItemPrimaryAssetType(TEXT("FTItemItem"));
	const FName FTItemDataDirectory(TEXT("/Game/Blueprints/Items/Data"));

	UFTItemDataAsset* FindItemDataByScanningDirectory(FName ItemId)
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		FARFilter Filter;
		Filter.PackagePaths.Add(FTItemDataDirectory);
		Filter.ClassPaths.Add(UFTItemDataAsset::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> ItemAssetDataList;
		AssetRegistry.GetAssets(Filter, ItemAssetDataList);
		for (const FAssetData& ItemAssetData : ItemAssetDataList)
		{
			UFTItemDataAsset* ItemDataAsset = Cast<UFTItemDataAsset>(ItemAssetData.GetAsset());
			if (ItemDataAsset && ItemDataAsset->ItemData.ItemId == ItemId)
			{
				return ItemDataAsset;
			}
		}

		return nullptr;
	}
}

UFTItemDataAsset* UFTItemFunctionLibrary::FindItemData(const UObject* WorldContextObject, FName ItemId)
{
	if (ItemId.IsNone())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetId AssetId = FPrimaryAssetId(FTItemPrimaryAssetType, ItemId);
	
	// 1. 이미 메모리에 로드되어 있는지 확인
	UObject* AssetObj = AssetManager.GetPrimaryAssetObject(AssetId);
	if (!AssetObj)
	{
		// 2. 로드되어 있지 않다면 에셋 매니저가 스캔한 경로를 통해 동기식으로 로드(Fallback)
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		if (AssetPath.IsValid())
		{
			AssetObj = AssetPath.TryLoad();
		}
	}
	
	if (UFTItemDataAsset* ItemDataAsset = Cast<UFTItemDataAsset>(AssetObj))
	{
		return ItemDataAsset;
	}

	return FindItemDataByScanningDirectory(ItemId);
}

int32 UFTItemFunctionLibrary::GetPlayerItemQuantity(const UObject* WorldContextObject, FName ItemId)
{
	if (!WorldContextObject || ItemId.IsNone())
	{
		return 0;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UFTInventoryComponent* InvComp = Pawn->FindComponentByClass<UFTInventoryComponent>())
			{
				return InvComp->GetItemQuantity(ItemId);
			}
		}
	}

	return 0;
}

FVector UFTItemFunctionLibrary::CalculateDropLocation(AActor* InstigatorActor)
{
	if (!InstigatorActor) return FVector::ZeroVector;

	FVector StartLoc = InstigatorActor->GetActorLocation();
	FVector ForwardDir = InstigatorActor->GetActorForwardVector();

	// 전방 120cm 평균, 좌우 -90cm ~ 90cm 오프셋
	FVector DropOffset = (ForwardDir * FMath::FRandRange(100.0f, 140.0f)) + (InstigatorActor->GetActorRightVector() * FMath::FRandRange(-90.0f, 90.0f));
	FVector TargetLoc = StartLoc + DropOffset;

	FHitResult HitResult;
	FVector TraceStart = TargetLoc + FVector(0.0f, 0.0f, 50.0f);
	FVector TraceEnd = TargetLoc - FVector(0.0f, 0.0f, 500.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InstigatorActor);

	UWorld* World = InstigatorActor->GetWorld();
	if (World && World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return HitResult.Location + FVector(0.0f, 0.0f, 15.0f);
	}

	return TargetLoc;
}
