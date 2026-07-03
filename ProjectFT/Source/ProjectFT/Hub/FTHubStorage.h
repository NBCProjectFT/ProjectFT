// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubStorage.generated.h"

class UFTInventoryComponent;
UCLASS()
class PROJECTFT_API AFTHubStorage : public AActor,  public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubStorage();

	bool AddStorageItem(FName ItemID, int32 Count);

	bool RemoveStorageItem(FName ItemID, int32 Count);

	int32 GetStorageItemCount(FName ItemID) const;

	const TArray<FTStorageItemStruct>& GetStorageItems() const;

	bool StoreItemFromInventory(UFTInventoryComponent* SourceInventory, FName ItemID, int32 Count);

	bool TakeItemToInventory(UFTInventoryComponent* TargetInventory, FName ItemID, int32 Count);

	void PrintStorageItems() const;
	
	UFTInventoryComponent* GetStorageInventory() const;
	
	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Storage|UI")
	void CloseStorageWidget();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="Storage",meta = (AllowPrivateAccess = "true"))
	UFTInventoryComponent * StorageInventory;
	
	UPROPERTY(EditAnywhere, Category = "Storage|Test")
	TArray<FTStorageItemStruct> TestStorageItems;
	
	UPROPERTY(Transient)
	mutable TArray<FTStorageItemStruct> CachedStorageItems;
	
	

	

	void OpenStorageWidget(AActor* Interactor);
};
