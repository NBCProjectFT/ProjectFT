#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FTItemIconEditorLibrary.generated.h"

class UFTItemDataAsset;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;
class UTextureRenderTarget2D;
class AActor;

UCLASS()
class PROJECTFTEDITOR_API UFTItemIconEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 아이템 아이콘 생성에 사용할 스태틱 메시를 아이템 데이터 에셋에서 로드합니다.
	 *
	 * @param ItemDataAsset ItemData.ItemMesh 참조를 가진 아이템 데이터 에셋입니다.
	 * @return 로드된 스태틱 메시입니다. 데이터 에셋이나 메시 참조가 유효하지 않으면 nullptr를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UStaticMesh* GetItemIconMesh(const UFTItemDataAsset* ItemDataAsset);

	/**
	 * 현재 에디터 월드에 임시 프리뷰 액터를 생성합니다.
	 *
	 * Editor Utility Widget은 일반 게임 월드 컨텍스트가 없을 수 있으므로,
	 * 일반 블루프린트 SpawnActor 호출이 실패하는 에디터 전용 아이콘 캡처 흐름에서 사용합니다.
	 *
	 * @param ActorClass 생성할 액터 클래스입니다.
	 * @param Location 프리뷰 액터의 월드 위치입니다.
	 * @param Rotation 프리뷰 액터의 월드 회전입니다.
	 * @param bTransient true면 생성한 액터를 레벨에 저장되지 않는 임시 객체로 표시합니다.
	 * @return 생성된 프리뷰 액터입니다. 에디터 월드나 클래스가 유효하지 않으면 nullptr를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static AActor* SpawnEditorPreviewActor(
		TSubclassOf<AActor> ActorClass,
		const FVector& Location,
		const FRotator& Rotation,
		bool bTransient = true);

	/**
	 * 아이콘 생성을 위해 만든 에디터 프리뷰 액터를 제거합니다.
	 *
	 * @param Actor 에디터 월드에서 제거할 액터입니다.
	 * @return 액터를 제거했으면 true, 실패했으면 false를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static bool DestroyEditorPreviewActor(AActor* Actor);

	/**
	 * 아이템 데이터 에셋으로부터 아이콘 텍스처를 생성하고, 생성된 텍스처를 다시 데이터 에셋에 할당합니다.
	 *
	 * ItemData.ItemMesh를 로드한 뒤 에디터 월드에 임시 프리뷰 액터를 생성하고,
	 * 임시 렌더 타겟으로 캡처한 결과를 Texture2D 에셋으로 저장합니다.
	 * 저장된 텍스처는 ItemData.ItemIcon에 자동 할당되며, 작업 후 프리뷰 액터는 제거됩니다.
	 *
	 * 아이콘 구도는 FTItemDataStruct에 저장된 아이템별 보정값인
	 * IconMeshRotation, IconCameraDistanceMultiplier, IconFOV를 사용합니다.
	 *
	 * @param ItemDataAsset 아이콘을 생성할 아이템 데이터 에셋입니다.
	 * @param PackagePath 생성된 텍스처 에셋을 저장할 콘텐츠 경로입니다. 예: "/Game/Blueprints/MakeIcons"
	 * @param AssetNameOverride 선택적 에셋 이름입니다. 비워두면 "T_Icon_" + ItemId를 사용하고, ItemId가 없으면 데이터 에셋 이름을 사용합니다.
	 * @param TextureSize 정사각형 출력 텍스처의 픽셀 크기입니다. 내부에서 허용 범위로 보정됩니다.
	 * @param bSavePackage true면 생성된 텍스처 패키지와 수정된 아이템 데이터 패키지를 즉시 저장합니다.
	 * @param MaskMaterial 배경 투명화를 위한 마스크 캡처용 머티리얼입니다. 비워두면 불투명 흰 배경으로 저장합니다.
	 * @return 생성된 Texture2D 에셋입니다. 생성에 실패하면 nullptr를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UTexture2D* GenerateItemIcon(
		UFTItemDataAsset* ItemDataAsset,
		const FString& PackagePath = TEXT("/Game/Blueprints/MakeIcons"),
		const FString& AssetNameOverride = TEXT(""),
		int32 TextureSize = 512,
		bool bSavePackage = true,
		UMaterialInterface* MaskMaterial = nullptr);

	/**
	 * 렌더 타겟을 Texture2D 에셋으로 저장합니다.
	 *
	 * 게임 스레드에서 렌더 타겟 픽셀을 읽고, 대상 Texture2D 에셋을 생성하거나 재사용합니다.
	 * 이후 소스 데이터를 초기화하고 패키지를 dirty 상태로 표시한 뒤, 필요하면 즉시 저장합니다.
	 *
	 * @param RenderTarget 픽셀을 읽을 렌더 타겟입니다.
	 * @param PackagePath 텍스처 에셋을 저장할 콘텐츠 경로입니다.
	 * @param AssetName 생성하거나 덮어쓸 텍스처 에셋 이름입니다.
	 * @param bSavePackage true면 텍스처 패키지를 즉시 저장합니다.
	 * @return 생성되었거나 갱신된 Texture2D 에셋입니다. 저장에 실패하면 nullptr를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static UTexture2D* SaveRenderTargetAsTextureAsset(
		UTextureRenderTarget2D* RenderTarget,
		const FString& PackagePath,
		const FString& AssetName,
		bool bSavePackage = true);

	/**
	 * 생성된 아이콘 텍스처를 아이템 데이터 에셋에 할당합니다.
	 *
	 * @param ItemDataAsset 수정할 아이템 데이터 에셋입니다.
	 * @param IconTexture ItemData.ItemIcon에 할당할 텍스처입니다.
	 * @param bSavePackage true면 수정된 아이템 데이터 에셋 패키지를 즉시 저장합니다.
	 * @return 텍스처를 성공적으로 할당했으면 true, 실패했으면 false를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectFT|Item Icons")
	static bool AssignItemIconTexture(
		UFTItemDataAsset* ItemDataAsset,
		UTexture2D* IconTexture,
		bool bSavePackage = true);
};
