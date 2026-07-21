#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "TimerManager.h"
#include "FTSecurityCharacter.generated.h"

class USceneComponent;
class UGameplayAbility;
class UWidgetComponent;

UCLASS()
class PROJECTFT_API AFTSecurityCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter();
	
	/** 보안요원이 지원요청 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|State")
	bool bIsRequestingSupport = false;
	
	/** 보안요원이 플레이어에게 접근하는 애니메이션 상태다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|State")
	bool bIsApproachingTarget = false;
	
	/** 보안요원이 플레이어를 붙잡은 애니메이션 상태다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|State")
	bool bIsGrabbing = false;
	
	/** 보안요원이 공격 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|State")
	bool bIsAttacking = false;
	
	/** 보안요원이 공격 사이 대기 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|State")
	bool bIsAttackDelay = false;
	
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
	virtual void OnDeath() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 붙잡힌 플레이어가 Attach 또는 추종할 위치 기준이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	TObjectPtr<USceneComponent> CapturePoint;

	/** 보안요원이 지원요청 중일 때 머리 위에 표시할 게이지 위젯이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Call")
	TObjectPtr<UWidgetComponent> SecurityCallGaugeWidgetComponent;

	/** BeginPlay에서 이 경비 ASC에 부여할 어빌리티들(잡기 UFTGA_Grab 등). 경비만 이 능력을 쓰므로 여기서 부여한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Security|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

private:
	FTimerHandle PawnCollisionRestoreTimerHandle;
	ECollisionResponse DefaultPawnCollisionResponse = ECR_Block;
};
