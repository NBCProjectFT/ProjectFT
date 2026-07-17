#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTHubStorage.generated.h"

class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API AFTHubStorage : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubStorage();

	UFTInventoryComponent* GetStorageInventory() const;

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Storage|UI")
	void CloseStorageWidget();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage", meta = (AllowPrivateAccess = "true"))
	UFTInventoryComponent* StorageInventory;

	UPROPERTY(EditAnywhere, Category = "Storage|Initial")
	TArray<FTStorageItemStruct> InitialItems;

	void OpenStorageWidget(AActor* Interactor);
};
