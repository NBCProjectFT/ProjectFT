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
	const bool bSavePackage)
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
	RenderTarget->ClearColor = FLinearColor::White;
	RenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	RenderTarget->InitAutoFormat(ClampedTextureSize, ClampedTextureSize);
	RenderTarget->UpdateResourceImmediate(true);

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
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetStaticMesh(ItemMesh);
	MeshComponent->RegisterComponent();
	MeshComponent->SetWorldRotation(ItemDataAsset->ItemData.IconMeshRotation);
	MeshComponent->UpdateBounds();

	const FBoxSphereBounds Bounds = MeshComponent->Bounds;
	const FVector TargetLocation = Bounds.Origin;
	const float Radius = FMath::Max(Bounds.SphereRadius, 50.0f);
	const float CameraDistanceMultiplier = FMath::Max(ItemDataAsset->ItemData.IconCameraDistanceMultiplier, 0.1f);
	const float CameraDistance = Radius * CameraDistanceMultiplier;
	const FVector CameraLocation = TargetLocation + FVector(-CameraDistance, -CameraDistance, CameraDistance * 0.55f);
	const FRotator CameraRotation = FRotationMatrix::MakeFromX(TargetLocation - CameraLocation).Rotator();

	UDirectionalLightComponent* LightComponent = NewObject<UDirectionalLightComponent>(PreviewActor, TEXT("IconPreviewLight"));
	PreviewActor->AddInstanceComponent(LightComponent);
	LightComponent->SetMobility(EComponentMobility::Movable);
	LightComponent->SetIntensity(8.0f);
	LightComponent->SetCastShadows(false);
	LightComponent->SetWorldRotation(FRotator(-45.0f, -35.0f, 0.0f));
	LightComponent->RegisterComponent();

	USkyLightComponent* SkyLightComponent = NewObject<USkyLightComponent>(PreviewActor, TEXT("IconPreviewSkyLight"));
	PreviewActor->AddInstanceComponent(SkyLightComponent);
	SkyLightComponent->SetMobility(EComponentMobility::Movable);
	SkyLightComponent->SetIntensity(2.5f);
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

	SavedTexture = SaveRenderTargetAsTextureAsset(
		RenderTarget,
		PackagePath,
		MakeIconAssetName(ItemDataAsset, AssetNameOverride),
		bSavePackage);

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

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		return nullptr;
	}

	TArray<FColor> PixelData;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(true);

	const FIntRect SourceRect(0, 0, RenderTarget->SizeX, RenderTarget->SizeY);
	if (!RenderTargetResource->ReadPixels(PixelData, ReadFlags, SourceRect) ||
		PixelData.Num() != RenderTarget->SizeX * RenderTarget->SizeY)
	{
		return nullptr;
	}

	for (FColor& Pixel : PixelData)
	{
		Pixel.A = 255;
	}

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
		RenderTarget->SizeX,
		RenderTarget->SizeY,
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
