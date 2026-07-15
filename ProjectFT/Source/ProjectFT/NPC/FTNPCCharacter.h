#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "FTNPCCharacter.generated.h"

class UWidgetComponent;

UCLASS()
class PROJECTFT_API AFTNPCCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTNPCCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsFleeing = false;

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|State")
	void SetIsFleeing(bool bNewIsFleeing);

protected:
	virtual void BeginPlay() override;
	virtual void OnImmobilizedStateChanged(bool bImmobilized) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|UI")
	TObjectPtr<UWidgetComponent> ReportGaugeWidgetComponent;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
