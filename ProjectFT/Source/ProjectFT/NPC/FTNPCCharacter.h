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
	
	/** 손님이 쇼핑 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsShopping = false;
	
	/** 손님이 의심 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsSuspicious = false;
	
	/** 손님이 신고 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsReporting = false;
	
	/** 손님이 공포 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsPanicked = false;
	
	/** 손님이 도망 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsFleeing = false;
	
	/** 손님이 도망친 위치에서 대기하는 애니메이션 상태다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsFleeWaiting = false;
	
	/** 손님이 무력화 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|NPC|State")
	bool bIsKnockedOut = false;

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|State")
	void SetIsFleeing(bool bNewIsFleeing);

protected:
	virtual void BeginPlay() override;
	virtual void OnDeath() override;
	virtual void OnImmobilizedStateChanged(bool bImmobilized) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|UI")
	TObjectPtr<UWidgetComponent> ReportGaugeWidgetComponent;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
