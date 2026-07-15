#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTHubRaidEntrance.generated.h"

class UFTRaidSelectWidget;
class UFTInventoryComponent;
class UTexture2D;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTRaidEntranceOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	TSoftObjectPtr<UTexture2D> PreviewImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	FName LevelName = NAME_None;

	/** None means free entry. Otherwise one item is consumed when travel is confirmed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	FName RequiredItemId = NAME_None;
};

UCLASS()
class PROJECTFT_API AFTHubRaidEntrance : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubRaidEntrance();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Hub|Flow")
	void RequestStartRaid(AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Hub|Raid")
	bool TryEnterRaid(FName LevelName, UFTInventoryComponent* PlayerInventory);

	const TArray<FFTRaidEntranceOption>& GetRaidOptions() const { return RaidOptions; }
	TSubclassOf<UFTRaidSelectWidget> GetRaidSelectWidgetClass() const { return RaidSelectWidgetClass; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Flow")
	FText InteractionPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	TArray<FFTRaidEntranceOption> RaidOptions;

	/** WBP class derived from UFTRaidSelectWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub|Raid")
	TSubclassOf<UFTRaidSelectWidget> RaidSelectWidgetClass;
};
