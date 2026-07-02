#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTCharacterBase.h"
#include "FTAICharacterBase.generated.h"


/**
 * AI 캐릭터들의 공통 기반 클래스
 * NPC, 보안요원 AI 캐릭터가 상속받는 베이스 클래스이다.
 */

UCLASS(Abstract)
class PROJECTFT_API AFTAICharacterBase : public AFTCharacterBase
{
	GENERATED_BODY()

public:
	AFTAICharacterBase();
	
	/**
	 * AI의 이동 속도를 변경한다.
	 * CharacterMovementComponent의 MaxWalkSpeed를 변경하는 용도로 사용된다.
	 *
	 * @param NewSpeed 새로 적용할 이동 속도
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|AI|Movement")
	virtual void SetMoveSpeed(float NewSpeed);

protected:
	/** 게임 시작 시 초기 체력 및 이동 속도를 적용 */
	virtual void BeginPlay() override;
	
	/** AI가 사망했을 때의 처리 로직 */
	virtual void HandleDeath() override;

	/** AI의 시작 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Stat", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	/** AI의 기본 이동 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Movement", meta = (ClampMin = "0.0"))
	float InitialMoveSpeed = 300.0f;
};
