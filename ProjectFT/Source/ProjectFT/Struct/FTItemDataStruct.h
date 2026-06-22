#pragma once

#include "CoreMinimal.h"
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
	int32 MaxStack = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UStaticMesh> ItemMesh;
};
