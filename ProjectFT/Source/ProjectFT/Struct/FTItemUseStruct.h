#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTItemUseStruct.generated.h"

class UFTGameplayAbility;
class UGameplayEffect;
class USoundBase;

/**
 * 아이템 "사용 행동" 데이터. 소비형 아이템에서 FTItemDataStruct.UseData로 채운다.
 * 범용 UFTGA_UseItem이 이 값을 읽어 GE를 적용하고 쿨다운을 건다 — 아이템마다 GA를 따로 만들지 않는다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FTItemUseStruct
{
	GENERATED_BODY()

public:
	// 사용 시 발동할 GameplayAbility(보통 UFTGA_UseItem 계열). 비우면 '사용 불가' 아이템.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use")
	TSubclassOf<UFTGameplayAbility> UseAbility;

	// 사용 시 대상(자신)에게 적용할 효과(들). 회복+해독처럼 복수도 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use")
	TArray<TSubclassOf<UGameplayEffect>> UseEffects;

	// 위 GE들에 주입할 SetByCaller 수치. 키 = Data.* 태그(예: Data.Heal), 값 = 크기(예: 회복량 50).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use")
	TMap<FGameplayTag, float> EffectMagnitudes;

	// 시전 시간(초). 0이면 즉시 적용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use", meta = (ClampMin = "0.0"))
	float CastTimeSeconds = 0.0f;

	// 쿨다운(초). 0이면 쿨다운 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

	// 쿨다운 식별 태그(아이템별 분리용). 같은 태그를 쓰는 아이템끼리는 쿨다운을 공유한다.
	// 비우면 공용 폴백(Cooldown.ItemUse) — 태그 미지정 아이템끼리 한 묶음으로 쿨다운을 공유한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	// 사용 시 사용자에게서 재생할 효과음. 비우면 무음.
	// GameplayCue가 아니라 여기 있는 이유: GE(UFTGE_Heal 등)는 아이템끼리 공유하므로 GE에 큐를 달면
	// 붕대·포션·에너지드링크가 전부 같은 소리를 낸다. 같은 GA/GE를 써도 다른 물건이라는 게 이 구조의 전제라,
	// UseEffects·EffectMagnitudes와 같은 층(= 아이템 데이터)에 둔다.
	// 재생은 UFTGA_ItemAbility::PlayUseSound가 담당하며, 시점은 CommitAbility 성공 직후다
	// (= 비용/쿨다운을 실제로 지불한 순간. 시전 취소나 비용 부족으로 끝나면 소리도 나지 않는다).
	// 대상에게 '벌어지는' 연출(테이저 임팩트 등)은 여전히 GameplayCue의 몫 — 층이 다르다.
	// 투척 아이템의 경우 이 소리는 '던지는' 소리다. 착탄음은 투사체 쪽 데이터가 담당한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Use")
	TObjectPtr<USoundBase> UseSound = nullptr;
};
