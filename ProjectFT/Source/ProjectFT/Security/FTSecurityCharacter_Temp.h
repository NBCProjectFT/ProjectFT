#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTCharacterBase.h"
#include "FTSecurityCharacter_Temp.generated.h"

/**
 * [Temp/모의] AFTSecurityCharacter를 AFTCharacterBase로 이관하는 패턴 검증용 사본(원본 미수정).
 * GAS 배선·MoveSpeed→MaxWalkSpeed 파생·스턴 시 이동 정지/복원까지 전부 베이스가 제공한다.
 * 그래서 이 클래스엔 'AI 고유'만 남는다 — 초기 스탯 + (예시) 사망 반응. 공격/추격 등은 담당자 몫.
 */
UCLASS()
class PROJECTFT_API AFTSecurityCharacter_Temp : public AFTCharacterBase
{
	GENERATED_BODY()

public:
	AFTSecurityCharacter_Temp();

	// 이동속도를 바꾼다. CMC를 직접 만지지 않고 MoveSpeed 속성으로 바꾸면 베이스가 MaxWalkSpeed에 반영하고 버프와 합성된다.
	UFUNCTION(BlueprintCallable)
	void SetMoveSpeed(float NewSpeed);

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 체력 0 도달 시 호출(베이스가 OnOutOfHealth로 통지). (예시) 경비 사망 처리.
	virtual void HandleDeath() override;

	// 초기 체력. 공용 속성셋 기본값(100) 위에 경비 수치로 덮어쓴다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat", meta = (ClampMin = "1.0"))
	float InitialHealth = 150.0f;

	// 경비 고유 이동속도(cm/s). 이 한 곳에만 둔다 — MoveSpeed 속성 base로 주입하면 베이스가 MaxWalkSpeed로 파생한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float InitialMoveSpeed = 300.0f;
};
