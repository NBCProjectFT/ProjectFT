#pragma once

#include "CoreMinimal.h"
#include "FTStruggleGaugeStruct.generated.h"

/**
 * 좌우 연타(발버둥) 탈출 게이지의 공용 로직. 잡기(FTCaptureEscapeComponent)·비눗방울 등 '연타로 탈출하는'
 * 모든 상태가 이 구조체를 합성해 재사용한다(게이지 계산을 효과마다 복붙하지 않는다).
 *
 *  - AddFlip()  : 연타(좌우 전환) 1회의 능동 탈출력.
 *  - Advance(dt): 매 틱 수동 증가(PassiveGainPerSecond) vs 저지력(DecayPerSecond)의 힘싸움.
 *  - IsFull()   : 임계값 도달(=탈출). 실제 해제/성공 처리는 이 구조체를 소유한 쪽이 한다.
 *
 * flip 판정(데드존/방향 전환)은 입력측(플레이어)이 하고, 여기서는 "연타 1회"만 받는다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FTStruggleGaugeStruct
{
	GENERATED_BODY()

public:
	// [능동] 좌우 전환 1회로 차는 양. 이 값이 클수록 잘 빠져나온다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struggle", meta = (ClampMin = "0.0"))
	float GainPerFlip = 1.0f;

	// [수동/자연증가] 연타와 무관하게 초당 저절로 차오르는 양. 0이면 순수 연타 대결.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Struggle", meta = (ClampMin = "0.0"))
	float PassiveGainPerSecond = 0.0f;

	// 챌린지 시작: 임계값·저지력을 주입하고 누적을 리셋한다. (GainPerFlip/Passive는 소유자가 세팅한 값을 유지.)
	void Begin(float InThreshold, float InDecayPerSecond)
	{
		Threshold = FMath::Max(InThreshold, KINDA_SMALL_NUMBER);
		DecayPerSecond = FMath::Max(InDecayPerSecond, 0.0f);
		Accumulated = 0.0f;
	}

	void Reset()
	{
		Accumulated = 0.0f;
	}

	// 연타(flip) 1회 반영.
	void AddFlip()
	{
		Accumulated = FMath::Clamp(Accumulated + GainPerFlip, 0.0f, Threshold);
	}

	// 시간 경과에 따른 수동 증가 vs 저지력. 순증가면 저절로 차고 순감소면 깎인다.
	void Advance(float DeltaSeconds)
	{
		const float NetPerSecond = PassiveGainPerSecond - DecayPerSecond;
		if (NetPerSecond != 0.0f)
		{
			Accumulated = FMath::Clamp(Accumulated + NetPerSecond * DeltaSeconds, 0.0f, Threshold);
		}
	}

	bool IsFull() const { return Accumulated >= Threshold; }

	// UI용 진행 비율(0..1).
	float GetProgress() const { return Threshold > 0.0f ? Accumulated / Threshold : 1.0f; }
	float GetAccumulated() const { return Accumulated; }
	float GetThreshold() const { return Threshold; }

private:
	// [주입] 탈출에 필요한 총량(소유자가 Begin으로 주입 — 잡기는 AI 붙잡는 힘).
	UPROPERTY(Transient)
	float Threshold = 1.0f;

	// [주입] 초당 자연 감소(저지력 — 잡기는 AI 탈출 저지력). Begin으로 주입.
	UPROPERTY(Transient)
	float DecayPerSecond = 0.0f;

	// 현재 누적(0..Threshold).
	UPROPERTY(Transient)
	float Accumulated = 0.0f;
};
