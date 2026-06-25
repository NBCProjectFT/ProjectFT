#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "FTHubQuestBoard.generated.h"

class AFTHubStorage;
class AFTHubShop;
class UFTHubQuestTestWidget;
class UDataTable;
class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API AFTHubQuestBoard : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubQuestBoard();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	bool CanCompleteQuest(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory);

	void GetQuestList(TArray<FTQuestStruct>& OutQuests) const;

	AFTHubStorage* GetHubStorage() const;

	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void CloseQuestWidget();
	
	bool IsQuestAvailable(FName QuestID) const;
	bool IsQuestCompleted(FName QuestID) const;
	EFTQuestStateType GetQuestState(FName QuestID) const;
	void UnlockQuest(FName QuestID);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	UDataTable* QuestDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quest")
	AFTHubStorage* HubStorage;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quest")
	AFTHubShop* HubShop;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FName> InitialQuestIDs;

	UPROPERTY(VisibleAnywhere, Category = "Quest")
	TSet<FName> AvailableQuestIDs;

	UPROPERTY(VisibleAnywhere, Category = "Quest")
	TSet<FName> CompletedQuestIDs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|UI")
	TSubclassOf<UFTHubQuestTestWidget> HubQuestTestWidgetClass;

private:
	void OpenQuestWidget(AActor* Interactor);

	const FTQuestStruct* FindQuestByID(FName QuestID) const;
	UFTInventoryComponent* FindPlayerInventory(AActor* Interactor) const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count);

	UPROPERTY(Transient)
	UFTHubQuestTestWidget* HubQuestTestWidget;
};
