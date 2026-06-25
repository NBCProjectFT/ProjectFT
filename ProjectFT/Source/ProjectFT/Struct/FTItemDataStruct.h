#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTItemDataStruct.generated.h"

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UStaticMesh> ItemMesh;

	// 사용 행동(어떤 GA로, 어떤 GE를, 어떤 수치/시전/쿨다운으로). 소비형 아이템만 채운다.
	// 범용 UFTGA_UseItem이 이 값을 읽어 동작 — 아이템마다 GA를 따로 만들지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FTItemUseStruct UseData;
};
