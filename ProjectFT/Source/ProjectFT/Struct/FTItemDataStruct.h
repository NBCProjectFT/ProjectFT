#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Enum/FTWeaponStanceType.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTItemDataStruct.generated.h"

class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct PROJECTFT_API FTItemDataStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FName ItemId = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float Weight = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int32 Cost = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	EFTItemCategoryType CategoryType = EFTItemCategoryType::None;

	// 이 아이템을 손에 들었을 때 AnimBP가 쓸 Idle/로코모션 스탠스. 무기가 아니면 Unarmed로 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Animation")
	EFTWeaponStanceType WeaponStance = EFTWeaponStanceType::Unarmed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TObjectPtr<UStaticMesh> ItemMesh;

	// Projectile 대신 인벤토리에 추가될 대체 일반/회복 아이템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta = (EditCondition = "CategoryType == EFTItemCategoryType::Projectile", AllowedClasses = "/Script/ProjectFT.FTItemDataAsset"))
	TSoftObjectPtr<UFTItemDataAsset> InventorySubstituteItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Appearance")
	FVector DropMeshScale = FVector(1.0f, 1.0f, 1.0f);

	//아이콘 만들 때 카메라 위치 조절용 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Icon") FRotator IconMeshRotation = FRotator(0.0f, -35.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Icon") FVector IconMeshLocationOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Icon") FVector IconCameraTargetOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Icon") float IconCameraDistanceMultiplier = 2.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Icon") float IconFOV = 28.0f;

	// 사용 행동(어떤 GA로, 어떤 GE를, 어떤 수치/시전/쿨다운으로). 소비형 아이템만 채운다.
	// 범용 UFTGA_UseItem이 이 값을 읽어 동작 — 아이템마다 GA를 따로 만들지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FTItemUseStruct UseData;
};
