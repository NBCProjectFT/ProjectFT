// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Struct/FTPlayerStatStruct.h"
#include "FTPlayerStatComponent.generated.h"

// 자원형 스탯(체력/스태미나) 변경 통지: (현재값, 최대값).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFTOnResourceStatChanged, float, Current, float, Max);
// 속성형 스탯(이동속도/손재주) 변경 통지: (새 값).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnAttributeStatChanged, float, NewValue);
// 체력이 0이 되어 사망.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnPlayerDied);

/**
 * 플레이어 스탯(체력/스태미나/이동속도/손재주)을 한곳에 모아 소유하고, 변경/회복/통지를 담당한다.
 * 값은 FFTPlayerStatStruct로 들고 있어 저장/리플리케이션에 통째로 쓰기 좋다.
 * 연속값(체력/스태미나 비율 등)은 getter 폴링, 변경 시점은 델리게이트로 UI에 전달한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTPlayerStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTPlayerStatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//~ Getter
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetHealth() const { return Stats.Health.Current; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetMaxHealth() const { return Stats.Health.Max; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetHealthFraction() const { return Stats.Health.GetFraction(); }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetStamina() const { return Stats.Stamina.Current; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetMaxStamina() const { return Stats.Stamina.Max; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetStaminaFraction() const { return Stats.Stamina.GetFraction(); }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetMoveSpeed() const { return Stats.MoveSpeed; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") float GetDexterity() const { return Stats.Dexterity; }
	UFUNCTION(BlueprintPure, Category = "FT|Stat") bool IsAlive() const { return bIsAlive; }

	//~ 체력
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void Heal(float Amount);
	// 체력 감소. 0이 되면 OnDied 브로드캐스트. (IFTDamageable 연동 시 여기로 포워딩)
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void ApplyDamage(float Amount);
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void SetMaxHealth(float NewMax, bool bRefill = false);

	//~ 스태미나
	// 충분하면 소모하고 true, 모자라면 소모하지 않고 false(점프 등 일괄 비용용).
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") bool TryConsumeStamina(float Amount);
	// 가능한 만큼만 깎고 0에서 멈춘다(스프린트처럼 매 프레임 지속 소모용).
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void DrainStamina(float Amount);
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void RestoreStamina(float Amount);
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void SetMaxStamina(float NewMax, bool bRefill = false);

	//~ 속성
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void SetMoveSpeed(float NewMoveSpeed);
	UFUNCTION(BlueprintCallable, Category = "FT|Stat") void SetDexterity(float NewDexterity);

	//~ 델리게이트(UI/게임플레이 바인딩)
	UPROPERTY(BlueprintAssignable, Category = "FT|Stat") FFTOnResourceStatChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "FT|Stat") FFTOnResourceStatChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "FT|Stat") FFTOnAttributeStatChanged OnMoveSpeedChanged;
	UPROPERTY(BlueprintAssignable, Category = "FT|Stat") FFTOnAttributeStatChanged OnDexterityChanged;
	UPROPERTY(BlueprintAssignable, Category = "FT|Stat") FFTOnPlayerDied OnDied;

protected:
	virtual void BeginPlay() override;

	// 초기 스탯 값(에디터에서 설정). 런타임 변경은 위 API를 통해 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat")
	FFTPlayerStatStruct Stats;

	// 스태미나 초당 회복량. 0이면 회복 안 함.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float StaminaRegenPerSecond = 15.0f;

	// 스태미나를 쓴 뒤 회복이 시작되기까지의 지연(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float StaminaRegenDelay = 1.0f;

	// 체력 초당 회복량. 0이면 회복 안 함(기본).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float HealthRegenPerSecond = 0.0f;

private:
	bool bIsAlive = true;

	// 마지막 스태미나 사용 후 경과 시간(회복 지연 판정용).
	float TimeSinceStaminaUse = 0.0f;
};
