#include "FTItemIconEditorLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "RenderingThread.h"
#include "UObject/Package.h"

namespace
{
	UWorld* GetProjectFTEditorWorld()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
		{
			return EditorWorld;
		}

		for (const FWorldContext& WorldContext : GEditor->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor && WorldContext.World())
			{
				return WorldContext.World();
			}
		}

		return nullptr;
	}

	FString MakeIconAssetName(const UFTItemDataAsset* ItemDataAsset, const FString& AssetNameOverride)
	{
		if (!AssetNameOverride.IsEmpty())
		{
			return AssetNameOverride;
		}

		if (ItemDataAsset && !ItemDataAsset->ItemData.ItemId.IsNone())
		{
			return FString::Printf(TEXT("T_Icon_%s"), *ItemDataAsset->ItemData.ItemId.ToString());
		}

		return ItemDataAsset
			? FString::Printf(TEXT("T_Icon_%s"), *ItemDataAsset->GetName())
			: TEXT("T_Icon_Item");
	}

	bool ReadRenderTargetPixels(UTextureRenderTarget2D* RenderTarget, TArray<FColor>& OutPixelData)
	{
		if (!RenderTarget)
		{
			return false;
		}

		FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		if (!RenderTargetResource)
		{
			return false;
		}

		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(true);

		const FIntRect SourceRect(0, 0, RenderTarget->SizeX, RenderTarget->SizeY);
		return RenderTargetResource->ReadPixels(OutPixelData, ReadFlags, SourceRect) &&
			OutPixelData.Num() == RenderTarget->SizeX * RenderTarget->SizeY;
	}

	UTexture2D* SavePixelsAsTextureAsset(
		const TArray<FColor>& PixelData,
		const int32 SizeX,
		const int32 SizeY,
		const FString& PackagePath,
		const FString& AssetName,
		const bool bSavePackage)
	{
		if (PixelData.Num() != SizeX * SizeY || PackagePath.IsEmpty() || AssetName.IsEmpty())
		{
			return nullptr;
		}

		const FString SanitizedAssetName = ObjectTools::SanitizeObjectName(AssetName);
		const FString SanitizedPackagePath = PackagePath.StartsWith(TEXT("/")) ? PackagePath : FString::Printf(TEXT("/Game/%s"), *PackagePath);
		const FString FullPackageName = FString::Printf(TEXT("%s/%s"), *SanitizedPackagePath, *SanitizedAssetName);

		if (!FPackageName::IsValidLongPackageName(FullPackageName))
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*FullPackageName);
		if (!Package)
		{
			return nullptr;
		}
		Package->FullyLoad();

		UTexture2D* Texture = FindObject<UTexture2D>(Package, *SanitizedAssetName);
		if (!Texture)
		{
			Texture = NewObject<UTexture2D>(
				Package,
				*SanitizedAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
		}

		if (!Texture)
		{
			return nullptr;
		}

		Texture->PreEditChange(nullptr);
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->Source.Init(
			SizeX,
			SizeY,
			1,
			1,
			TSF_BGRA8,
			reinterpret_cast<const uint8*>(PixelData.GetData()));
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		Package->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(Texture);

		if (bSavePackage)
		{
			UEditorLoadingAndSavingUtils::SavePackages({ Package }, true);
		}

		return Texture;
	}
}

UStaticMesh* UFTItemIconEditorLibrary::GetItemIconMesh(const UFTItemDataAsset* ItemDataAsset)
{
	if (!ItemDataAsset || ItemDataAsset->ItemData.ItemMesh.IsNull())
	{
		return nullptr;
	}

	return ItemDataAsset->ItemData.ItemMesh.LoadSynchronous();
}

AActor* UFTItemIconEditorLibrary::SpawnEditorPreviewActor(
	TSubclassOf<AActor> ActorClass,
	const FVector& Location,
	const FRotator& Rotation,
	const bool bTransient)
{
	if (!ActorClass)
	{
		return nullptr;
	}

	UWorld* EditorWorld = GetProjectFTEditorWorld();
	if (!EditorWorld)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (bTransient)
	{
		SpawnParameters.ObjectFlags |= RF_Transient;
	}

	return EditorWorld->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParameters);
}

bool UFTItemIconEditorLibrary::DestroyEditorPreviewActor(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	UWorld* World = Actor->GetWorld();
	return World ? World->EditorDestroyActor(Actor, false) : false;
}

UTexture2D* UFTItemIconEditorLibrary::GenerateItemIcon(
	UFTItemDataAsset* ItemDataAsset,
	const FString& PackagePath,
	const FString& AssetNameOverride,
	const int32 TextureSize,
	const bool bSavePackage,
	UMaterialInterface* MaskMaterial)
{
	UStaticMesh* ItemMesh = GetItemIconMesh(ItemDataAsset);
	if (!ItemDataAsset || !ItemMesh)
	{
		return nullptr;
	}

	UWorld* EditorWorld = GetProjectFTEditorWorld();
	if (!EditorWorld)
	{
		return nullptr;
	}

	const int32 ClampedTextureSize = FMath::Clamp(TextureSize, 64, 4096);
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	RenderTarget->ClearColor = FLinearColor(0.72f, 0.76f, 0.80f, 1.0f);
	RenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	RenderTarget->InitAutoFormat(ClampedTextureSize, ClampedTextureSize);
	RenderTarget->UpdateResourceImmediate(true);

	UTextureRenderTarget2D* MaskRenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	MaskRenderTarget->ClearColor = FLinearColor::Black;
	MaskRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	MaskRenderTarget->InitAutoFormat(ClampedTextureSize, ClampedTextureSize);
	MaskRenderTarget->UpdateResourceImmediate(true);

	AActor* PreviewActor = SpawnEditorPreviewActor(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, true);
	if (!PreviewActor)
	{
		return nullptr;
	}

	UTexture2D* SavedTexture = nullptr;

	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(PreviewActor, TEXT("IconPreviewMesh"));
	PreviewActor->SetRootComponent(MeshComponent);
	PreviewActor->AddInstanceComponent(MeshComponent);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(true);
	MeshComponent->SetStaticMesh(ItemMesh);
	MeshComponent->RegisterComponent();
	MeshComponent->SetWorldLocation(ItemDataAsset->ItemData.IconMeshLocationOffset);
	MeshComponent->SetWorldRotation(ItemDataAsset->ItemData.IconMeshRotation);
	MeshComponent->UpdateBounds();

	const FBoxSphereBounds Bounds = MeshComponent->Bounds;
	const FVector PivotLocation = MeshComponent->GetComponentLocation();
	const FVector TargetLocation = PivotLocation + ItemDataAsset->ItemData.IconCameraTargetOffset;
	const FVector BoundsMin = Bounds.Origin - Bounds.BoxExtent;
	const FVector BoundsMax = Bounds.Origin + Bounds.BoxExtent;
	float RadiusFromPivot = 0.0f;
	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector BoundsCorner(
					XIndex == 0 ? BoundsMin.X : BoundsMax.X,
					YIndex == 0 ? BoundsMin.Y : BoundsMax.Y,
					ZIndex == 0 ? BoundsMin.Z : BoundsMax.Z);
				RadiusFromPivot = FMath::Max(RadiusFromPivot, FVector::Distance(PivotLocation, BoundsCorner));
			}
		}
	}

	const float Radius = FMath::Max(RadiusFromPivot, 10.0f);
	const float CameraDistanceMultiplier = FMath::Max(ItemDataAsset->ItemData.IconCameraDistanceMultiplier, 0.01f);
	const float CameraDistance = Radius * CameraDistanceMultiplier;
	const FVector CameraLocation = TargetLocation + FVector(-CameraDistance * 0.45f, -CameraDistance * 0.45f, CameraDistance * 0.75f);
	const FRotator CameraRotation = FRotationMatrix::MakeFromX(TargetLocation - CameraLocation).Rotator();

	UDirectionalLightComponent* LightComponent = NewObject<UDirectionalLightComponent>(PreviewActor, TEXT("IconPreviewLight"));
	PreviewActor->AddInstanceComponent(LightComponent);
	LightComponent->SetMobility(EComponentMobility::Movable);
	LightComponent->SetIntensity(3.5f);
	LightComponent->SetCastShadows(true);
	LightComponent->SetWorldRotation(FRotator(-45.0f, -35.0f, 0.0f));
	LightComponent->RegisterComponent();

	USkyLightComponent* SkyLightComponent = NewObject<USkyLightComponent>(PreviewActor, TEXT("IconPreviewSkyLight"));
	PreviewActor->AddInstanceComponent(SkyLightComponent);
	SkyLightComponent->SetMobility(EComponentMobility::Movable);
	SkyLightComponent->SetIntensity(0.8f);
	SkyLightComponent->SetCastShadows(false);
	SkyLightComponent->RegisterComponent();

	USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>(PreviewActor, TEXT("IconPreviewCapture"));
	PreviewActor->AddInstanceComponent(CaptureComponent);
	CaptureComponent->SetMobility(EComponentMobility::Movable);
	CaptureComponent->TextureTarget = RenderTarget;
	CaptureComponent->CaptureSource = SCS_FinalColorLDR;
	CaptureComponent->FOVAngle = FMath::Clamp(ItemDataAsset->ItemData.IconFOV, 5.0f, 170.0f);
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->SetWorldLocation(CameraLocation);
	CaptureComponent->SetWorldRotation(CameraRotation);
	CaptureComponent->RegisterComponent();
	CaptureComponent->CaptureScene();
	FlushRenderingCommands();

	TArray<FColor> ColorPixels;
	if (ReadRenderTargetPixels(RenderTarget, ColorPixels))
	{
		if (MaskMaterial)
		{
			TArray<UMaterialInterface*> OriginalMaterials;
			const int32 MaterialCount = MeshComponent->GetNumMaterials();
			OriginalMaterials.Reserve(MaterialCount);
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				OriginalMaterials.Add(MeshComponent->GetMaterial(MaterialIndex));
				MeshComponent->SetMaterial(MaterialIndex, MaskMaterial);
			}

			CaptureComponent->TextureTarget = MaskRenderTarget;
			CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
			CaptureComponent->ShowOnlyComponent(MeshComponent);
			CaptureComponent->CaptureScene();
			FlushRenderingCommands();

			TArray<FColor> MaskPixels;
			if (ReadRenderTargetPixels(MaskRenderTarget, MaskPixels) && MaskPixels.Num() == ColorPixels.Num())
			{
				for (int32 PixelIndex = 0; PixelIndex < ColorPixels.Num(); ++PixelIndex)
				{
					ColorPixels[PixelIndex].A = MaskPixels[PixelIndex].R > 127 ? 255 : 0;
				}
			}
			else
			{
				for (FColor& Pixel : ColorPixels)
				{
					Pixel.A = 255;
				}
			}

			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				MeshComponent->SetMaterial(MaterialIndex, OriginalMaterials[MaterialIndex]);
			}
		}
		else
		{
			for (FColor& Pixel : ColorPixels)
			{
				Pixel.A = 255;
			}
		}

		SavedTexture = SavePixelsAsTextureAsset(
			ColorPixels,
			RenderTarget->SizeX,
			RenderTarget->SizeY,
			PackagePath,
			MakeIconAssetName(ItemDataAsset, AssetNameOverride),
			bSavePackage);
	}

	if (SavedTexture)
	{
		AssignItemIconTexture(ItemDataAsset, SavedTexture, bSavePackage);
	}

	DestroyEditorPreviewActor(PreviewActor);

	return SavedTexture;
}

UTexture2D* UFTItemIconEditorLibrary::SaveRenderTargetAsTextureAsset(
	UTextureRenderTarget2D* RenderTarget,
	const FString& PackagePath,
	const FString& AssetName,
	const bool bSavePackage)
{
	if (!RenderTarget || PackagePath.IsEmpty() || AssetName.IsEmpty())
	{
		return nullptr;
	}

	TArray<FColor> PixelData;
	if (!ReadRenderTargetPixels(RenderTarget, PixelData))
	{
		return nullptr;
	}

	for (FColor& Pixel : PixelData)
	{
		Pixel.A = 255;
	}

	return SavePixelsAsTextureAsset(PixelData, RenderTarget->SizeX, RenderTarget->SizeY, PackagePath, AssetName, bSavePackage);
}

bool UFTItemIconEditorLibrary::AssignItemIconTexture(
	UFTItemDataAsset* ItemDataAsset,
	UTexture2D* IconTexture,
	const bool bSavePackage)
{
	if (!ItemDataAsset || !IconTexture)
	{
		return false;
	}

	ItemDataAsset->Modify();
	ItemDataAsset->ItemData.ItemIcon = IconTexture;
	ItemDataAsset->MarkPackageDirty();

	if (bSavePackage)
	{
		UEditorLoadingAndSavingUtils::SavePackages({ ItemDataAsset->GetOutermost() }, true);
	}

	return true;
}
