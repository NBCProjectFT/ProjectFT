#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "FTHubShop.generated.h"

class UFTInventoryComponent;
class UFTHubShopWidget;

UCLASS()
class PROJECTFT_API AFTHubShop : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubShop();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopItems();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void UnlockShopItem(FName ItemID);

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsShopItemUnlocked(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const;

	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void CloseShopWidget();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|UI")
	TSubclassOf<UFTHubShopWidget> HubShopWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> FixedShopItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> RandomItemPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 RandomSlotCount = 6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FTShopItemStruct> CurrentShopItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TSet<FName> UnlockedShopItemIDs;

private:
	void OpenShopWidget(AActor* Interactor);
	const FTShopItemStruct* FindCurrentShopItem(FName ItemID) const;
	UFTInventoryComponent* FindPlayerInventory(AActor* Interactor) const;
	void PrintShopItems() const;

	UPROPERTY(Transient)
	UFTHubShopWidget* HubShopWidget;
};
