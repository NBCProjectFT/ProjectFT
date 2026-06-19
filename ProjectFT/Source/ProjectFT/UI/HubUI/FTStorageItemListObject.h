#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTStorageItemListObject.generated.h"

UCLASS()
class PROJECTFT_API UFTStorageItemListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTStorageItemStruct& InStorageItem);
	const FTStorageItemStruct& GetStorageItem() const;

private:
	FTStorageItemStruct StorageItem;
};
