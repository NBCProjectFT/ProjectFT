#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FTItemIconEditorLibrary.generated.h"

class UFTItemDataAsset;
class UStaticMesh;
class UTexture2D;
class UTextureRenderTarget2D;
class AActor;

UCLASS()
class PROJECTFTEDITOR_API UFTItemIconEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UStaticMesh* GetItemIconMesh(const UFTItemDataAsset* ItemDataAsset);

	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static AActor* SpawnEditorPreviewActor(
		TSubclassOf<AActor> ActorClass,
		const FVector& Location,
		const FRotator& Rotation,
		bool bTransient = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static bool DestroyEditorPreviewActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UTexture2D* GenerateItemIcon(
		UFTItemDataAsset* ItemDataAsset,
		const FString& PackagePath = TEXT("/Game/Blueprints/MakeIcons"),
		const FString& AssetNameOverride = TEXT(""),
		int32 TextureSize = 512,
		bool bSavePackage = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UTexture2D* SaveRenderTargetAsTextureAsset(
		UTextureRenderTarget2D* RenderTarget,
		const FString& PackagePath,
		const FString& AssetName,
		bool bSavePackage = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static bool AssignItemIconTexture(
		UFTItemDataAsset* ItemDataAsset,
		UTexture2D* IconTexture,
		bool bSavePackage = true);
};
