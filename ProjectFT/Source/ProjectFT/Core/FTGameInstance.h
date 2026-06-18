#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FTGameInstance.generated.h"

class UFTSaveGame;

UCLASS()
class PROJECTFT_API UFTGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "FT|Save")
	TObjectPtr<UFTSaveGame> CurrentSaveData = nullptr;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void StartRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void ReturnToBase();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void RequestSave();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void RequestLoad();
};
