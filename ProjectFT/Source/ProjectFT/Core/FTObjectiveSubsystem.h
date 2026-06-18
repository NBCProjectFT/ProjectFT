#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTObjectiveSubsystem.generated.h"

UCLASS()
class PROJECTFT_API UFTObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "FT|Objective")
	FName CurrentQuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Objective")
	TSet<FName> RequiredItems;

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	bool IsObjectiveCompleted() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	void NotifyItemPickedUp(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	void NotifyEscapeReached();

private:
	UPROPERTY()
	TSet<FName> PickedUpRequiredItems;
};
