#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTCharacterBase.h"
#include "FTAICharacterBase.generated.h"

/**
 * AI 캐릭터들의 공통 기반 클래스다.
 * 손님 NPC, 보안요원 등 AI 캐릭터가 상속받는다.
 */
UCLASS(Abstract)
class PROJECTFT_API AFTAICharacterBase : public AFTCharacterBase
{
	GENERATED_BODY()

public:
	AFTAICharacterBase();

	/** StateTree의 KnockedOut 연출이 끝난 뒤 AI를 제거한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|AI|Death")
	void DespawnAfterDeath();
	
	/**
	 * AI의 이동 속도를 변경한다.
	 * CharacterMovementComponent의 MaxWalkSpeed를 갱신할 때 사용한다.
	 *
	 * @param NewSpeed 새로 적용할 이동 속도
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|AI|Movement")
	virtual void SetMoveSpeed(float NewSpeed);

protected:
	/** 게임 시작 시 초기 체력과 이동 속도를 적용한다. */
	virtual void BeginPlay() override;
	virtual void OnDeath() override;

	/** AI의 시작 체력이다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Stat", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	/** AI의 기본 이동 속도다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|AI|Movement", meta = (ClampMin = "0.0"))
	float InitialMoveSpeed = 200.0f;

};
