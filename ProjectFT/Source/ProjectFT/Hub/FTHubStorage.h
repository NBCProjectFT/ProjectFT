// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTHubStorage.generated.h"

UCLASS()
class PROJECTFT_API AFTHubStorage : public AActor
{
	GENERATED_BODY()

public:
	AFTHubStorage();

	void AddStorageItem(FName ItemID, int32 Count);

	bool RemoveStorageItem(FName ItemID, int32 Count);

	int32 GetStorageItemCount(FName ItemID) const;

	const TArray<FTStorageItemStruct>& GetStorageItems() const;

	void PrintStorageItems() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Storage|Test")
	TArray<FTStorageItemStruct> TestStorageItems;
};
