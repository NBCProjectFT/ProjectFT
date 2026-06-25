#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 아이템 사용 어빌리티의 공용(폴백) 쿨다운 식별 태그. 쿨다운 GE가 적용되는 동안 소유자에게 부여되며,
// 호출측(AFTPlayerCharacter)이 이 태그(또는 아이템이 지정한 CooldownTag)로 재사용을 차단한다. 아이템이 CooldownTag를 지정하면 그게 우선.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Cooldown_ItemUse);

// 쿨다운 지속시간을 SetByCaller로 주입할 때 쓰는 데이터 태그(어빌리티별 CooldownSeconds 주입 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Cooldown);

// 지속형 GameplayEffect의 공용 지속시간을 SetByCaller로 주입할 때 쓰는 데이터 태그.
// 한 아이템에서 효과별 지속시간을 다르게 줘야 하는 경우에는 Data.StunDuration 같은 전용 태그를 별도로 쓴다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Duration);

// 아이템 사용 어빌리티가 시전(활성) 중인 동안 소유자에게 부여되는 상태 태그. 이동 입력 시 이 태그로 시전을 취소한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_UsingItem);

// ── 버프/디버프 상태 태그 ──────────────────────────────────────────────
// GE가 활성인 동안 대상에게 부여되어 "걸려있음"을 표현한다(ASC HasMatchingGameplayTag로 판정).
// 계층적이라 부모 태그로 일괄 처리 가능: State.Debuff 하나로 "디버프 있나?" 질의나, 해독 아이템의
// RemoveActiveEffectsWithGrantedTags(일괄 제거)가 자식(Poison 등)까지 매칭한다. State.Buff도 동일.
// 새 버프/디버프는 각각 State.Buff.* / State.Debuff.* 아래에 추가할 것.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff);          // 모든 디버프의 부모(질의/해독용)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Poison);   // 독: 주기적 체력 감소 (UFTGE_Poison)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Slow);     // 슬로우: 이동속도 감소 (UFTGE_Slow)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Buff);            // 모든 버프의 부모(질의/디스펠용)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Buff_Haste);     // 헤이스트: 이동속도 증가 (UFTGE_Haste)

// 아이템 사용 입력 → 사용 어빌리티(UFTGA_UseItem)를 발동시키는 GameplayEvent 태그. 페이로드 = 대상 UFTItemDataAsset.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_UseItem);

// 회복 효과(UFTGE_Heal)의 회복량을 SetByCaller로 주입할 때 쓰는 데이터 태그(아이템 데이터의 EffectMagnitudes 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Heal);

// 테이저(예시 전용 GA) — 사용 이벤트 트리거 / 스턴 상태(디버프) / 스턴 지속시간(SetByCaller 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_UseTaser);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Stun);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_StunDuration);

// 테이저 적중 연출 GameplayCue. C++가 임팩트 지점에서 ExecuteGameplayCue로 발동하고, 비주얼은 GC_TaserHit Notify(BP)가 담당한다.
// (GameplayCue.* 루트여야 GameplayCueManager가 Notify로 라우팅한다.)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_Taser_Hit);

// 스턴 상태 '지속' 연출 GameplayCue. 스턴 태그가 붙어있는 동안 캐릭터 ASC에 Add/Remove(베이스가 관리), 비주얼은 GC_Stun Notify(BP, 루핑)가 담당.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_State_Stun);

// 던지기(전용 GA) — 사용 이벤트 트리거.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_ThrowItem);

// GAS data and state tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Damage);

// Item classification tags. ItemDataAsset may own multiple types at once.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Misc);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Usable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Material);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Weapon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Armor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Consumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_RecipeBook);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Quest);
