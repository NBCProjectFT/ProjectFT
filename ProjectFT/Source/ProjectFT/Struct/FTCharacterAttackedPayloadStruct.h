#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTCharacterAttackedPayloadStruct.generated.h"

// 적대적 GE(Effect.Hostile 에셋 태그 보유 — 데미지/스턴/슬로우/독/비눗방울 등)가 대상에게 적용될 때 브로드캐스트되는 페이로드.
// "누가(Instigator) 누구를(Target) 어떤 효과로(EffectTags) 공격했나"의 단일 신호 — 어그로 판정의 소스.
// 공격자 정보는 GE Spec의 EffectContext에 이미 실려오므로 더미 속성 없이 그 컨텍스트에서 추출한다.
// 데미지 '수치'가 필요한 소비자(체력바/신고 리셋 등)는 이 채널이 아니라 Event.Character.Damaged를 쓴다.
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTCharacterAttackedPayloadStruct
{
	GENERATED_BODY()

	// 공격한 주체(무주체 효과 — 환경/트랩 등 — 이면 null일 수 있다).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Attacked")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	// 공격당한 대상.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Attacked")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 이 공격을 일으킨 GE의 태그(에셋 태그 + 부여 태그 병합). 리스너가 GE 클래스를 몰라도
	// 공격 종류를 태그로 분기한다(예: EffectTags.HasTag(State.Debuff.Stun)로 "스턴 공격"인지 판별).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Character|Attacked")
	FGameplayTagContainer EffectTags;
};
