#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "TimerManager.h"
#include "FTSecurityCharacter.generated.h"

class USceneComponent;

UCLASS()
class PROJECTFT_API AFTSecurityCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter();
	
	virtual void SetMoveSpeed(float NewSpeed) override;
	void IgnorePawnCollisionForDuration(float Duration);
	void SetPawnCollisionIgnored(bool bIgnored);
	void RestorePawnCollision();

	/** 붙잡힌 대상이 Attach 또는 추종에 사용할 위치 기준을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Capture")
	USceneComponent* GetCapturePointComponent() const;

	/** 현재 CapturePoint의 월드 위치를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Capture")
	FVector GetCapturePointLocation() const;

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 붙잡힌 플레이어가 Attach 또는 추종할 위치 기준이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	TObjectPtr<USceneComponent> CapturePoint;

private:
	FTimerHandle PawnCollisionRestoreTimerHandle;
	ECollisionResponse DefaultPawnCollisionResponse = ECR_Block;
};
