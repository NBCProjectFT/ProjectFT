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

// 버블에 실제로 갇혀 있는 시간(UFTGE_BubbleTrap)을 SetByCaller로 주입할 때 쓰는 데이터 태그.
// 버블 스택 유지 시간은 공용 Data.Duration을 쓰고, 같은 DA에서 갇힘 시간까지 함께 줘야 하므로 별도 태그로 분리한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_BubbleDuration);

// 아이템 사용 어빌리티가 시전/조준(활성) 중인 동안 소유자에게 부여되는 상태 태그(ActivationOwnedTags).
// "아이템 동작 진행 중?"의 값싼 가드 질의(HasMatchingGameplayTag)에 쓴다. 실제 취소 매칭은 아래 Ability.ItemUse.* AssetTags로 한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_UsingItem);

// 아이템 사용 어빌리티의 식별(Asset) 태그. CancelAbilities는 ActivationOwnedTags가 아니라 "AssetTags"를 매칭하므로, 취소 대상은 이 태그로 구분한다.
//  - 부모 Ability.ItemUse        : 아이템 사용 동작 전반 — 퀵슬롯/메뉴 전환 시 종류 불문 취소(부모 질의가 자식까지 매칭).
//  - .Channeled (시전형, UseItem) : 이동 시에도 취소(채널링 중단).
//  - .Aimed     (조준형, ThrowItem): 이동해도 유지(달리며 던지기 가능), 퀵슬롯 전환에서만 취소.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Ability_ItemUse);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Ability_ItemUse_Channeled);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Ability_ItemUse_Aimed);

// 행동불능(이동정지+어빌리티차단)의 '우산' 태그. 모든 행동불능 효과(스턴/마비/빙결/석화/비눗방울 등)가
// 자기 식별 태그와 함께 이 태그를 추가로 부여한다. 캐릭터(이동 정지)와 UFTGameplayAbility(활성 차단)는
// 개별 효과가 아니라 오직 이 우산 태그만 감시하므로, 새 행동불능 효과를 추가해도 그 코드는 바뀌지 않는다(OCP).
// 카운트가 곧 '동시에 활성인 행동불능 수' — 하나 풀려도 남아있으면 유지, 전부 사라져야 해제된다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Immobilized);

// 적대적 행동(공격) 식별용 '에셋' 태그. 대상을 해치려는 의도의 GE라면 종류(데미지/스턴/슬로우/독/비눗방울 등)를
// 불문하고 이 태그를 자기 에셋 태그로 단다. 대상 캐릭터(AFTCharacterBase)는 GE 적용 시 이 태그 하나만 보고
// "공격당함"으로 간주해 어그로 신호(Event.Character.Attacked)를 발행한다 — 카테고리별 처리를 늘리지 않는 단일 판정 기준.
// (부여 태그 State.Debuff.*는 '대상의 상태'를 표현하고, 이 에셋 태그는 'GE의 의도'를 표현한다 — 층위가 다르다.)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Effect_Hostile);

// ── 버프/디버프 상태 태그 ──────────────────────────────────────────────
// GE가 활성인 동안 대상에게 부여되어 "걸려있음"을 표현한다(ASC HasMatchingGameplayTag로 판정).
// 계층적이라 부모 태그로 일괄 처리 가능: State.Debuff 하나로 "디버프 있나?" 질의나, 해독 아이템의
// RemoveActiveEffectsWithGrantedTags(일괄 제거)가 자식(Poison 등)까지 매칭한다. State.Buff도 동일.
// 새 버프/디버프는 각각 State.Buff.* / State.Debuff.* 아래에 추가할 것.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff);          // 모든 디버프의 부모(질의/해독용)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Poison);   // 독: 주기적 체력 감소 (UFTGE_Poison)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Slow);     // 슬로우: 이동속도 감소 (UFTGE_Slow)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Escapable);// GE 기반 자가 탈출형 디버프(버블/빙결 등): GA_EscapableDebuff 트리거/제거 기준
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Buff);            // 모든 버프의 부모(질의/디스펠용)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Buff_Haste);     // 헤이스트: 이동속도 증가 (UFTGE_Haste)

// 발버둥(좌우 연타) 입력 1회를 알리는 GameplayEvent. 플레이어가 flip을 감지할 때마다 자기 ASC로 발행하고,
// 현재 활성인 탈출 시스템(캡처 컴포넌트 또는 GE 기반 탈출 GA)이 이 이벤트를 받아 자기 게이지를 올린다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_Struggle);

// 사망 상태 태그. 체력 소진 시 AFTCharacterBase가 소유 ASC에 Loose 태그로 부여한다(GE 수명이 아닌 캐릭터 상태).
// "죽었나?" 질의(HasMatchingGameplayTag)와 사망 중 어빌리티 발동 차단(GA의 ActivationBlockedTags)의 단일 소스로 쓴다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Dead);

// 플레이어가 보안 요원에게 

// AI (보안요원)에게 플레이어가 붙잡혀 있는 상태.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Captured);

// 훔치는 채널형 상호작용(예: LootShelf) 진행 중 시전자(플레이어) ASC에 부여되는 상태 태그.
// UFTChanneledInteractionComponent가 ChannelingStateTag로 이 값을 부여/해제한다. NPC/경비 AI가 도둑질 인식에 질의한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Stealing);

// 경비 잡기 어빌리티(UFTGA_Grab) 발동용 GameplayEvent 트리거 태그. StateTree가 대상(플레이어)을 페이로드(Target)로 실어 보낸다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_Grab);

// 잡기 어빌리티가 활성인 동안 경비 ASC에 부여되는 진행 상태 태그(ActivationOwnedTags). StateTree가 "아직 잡는 중?" 질의에 쓴다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Grabbing);

// 아이템 사용 입력 → 사용 어빌리티(UFTGA_UseItem)를 발동시키는 GameplayEvent 태그. 페이로드 = 대상 UFTItemDataAsset.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_UseItem);

// 아이템 사용 입력에서 "손을 뗀 순간"을 알리는 제네릭 GameplayEvent 태그(트리거 태그 아님 — 어떤 어빌리티도 새로 발동시키지 않는다).
// 충전형(투척 등) 어빌리티가 활성 중 WaitGameplayEvent로 이 태그를 받아 실제 발동한다. 즉시형 어빌리티는 이미 종료돼 무해한 no-op.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_UseReleased);

// 회복 효과(UFTGE_Heal)의 회복량을 SetByCaller로 주입할 때 쓰는 데이터 태그(아이템 데이터의 EffectMagnitudes 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Heal);

// 테이저(예시 전용 GA) — 사용 이벤트 트리거 / 스턴 상태(디버프) / 스턴 지속시간(SetByCaller 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_UseTaser);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Stun);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_StunDuration);

// 마비 상태(디버프). 스턴과 동일하게 '행동불능'으로 취급된다 — 부여되는 동안 AFTCharacterBase가 이동을 정지시키고,
// 모든 FT 어빌리티 활성이 차단된다(스턴/마비 중 하나라도 있으면 정지 유지). 스턴과의 차이는 적용 주체/연출뿐.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Paralyzed);

// ── 비눗방울 갇힘 메커니즘(버블건) ─────────────────────────────────
// BubbleStack : 버블 피격 1회당 부여되는 '스택' 태그(UFTGE_BubbleStack, 짧은 지속 → 시간 지나면 개별 만료해 카운트 자연 감소).
//   이 태그의 '카운트'가 임계치에 도달하면 UFTGA_BubbleStackTrap이 갇힘 상태 GE(UFTGE_BubbleTrap)를 적용한다. 이 태그 자체는 이동/행동을 막지 않는다.
// Bubble      : 커다란 비눗방울에 '갇힌' 상태(Loose). 스턴/마비와 동일한 행동불능 — 아래 행동불능 태그 집합/차단에 포함된다.
// (형제 관계에 주의: "Bubble"과 "BubbleStack"은 계층 부모-자식이 아니라 별개 리프라 서로 카운트가 섞이지 않는다.)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_BubbleStack);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_Debuff_Bubble);

// 테이저 적중 연출 GameplayCue. C++가 임팩트 지점에서 ExecuteGameplayCue로 발동하고, 비주얼은 GC_TaserHit Notify(BP)가 담당한다.
// (GameplayCue.* 루트여야 GameplayCueManager가 Notify로 라우팅한다.)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_Taser_Hit);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_Taser_Start);

// 스턴 상태 '지속' 연출 GameplayCue. 스턴 태그가 붙어있는 동안 캐릭터 ASC에 Add/Remove(베이스가 관리), 비주얼은 GC_Stun Notify(BP, 루핑)가 담당.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_State_Stun);

// 비눗방울 갇힘 '지속' 연출 GameplayCue. UFTGE_BubbleTrap이 활성인 동안 자동 발동/제거된다(GE 수명과 함께).
// 비주얼(액터를 감싸는 큰 구체)은 GC_Bubble Notify(BP, 루핑 — GameplayCueNotify_Looping/Actor)가 담당한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_State_Bubble);

// 물리공격이 Hit했을 때 나타나는 VFX용 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_GameplayCue_Melee_Hit);

// 던지기(전용 GA) — 사용 이벤트 트리거.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_ThrowItem);

// 근접공격(전용 GA)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_Melee_Begin);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_Melee_End);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_Melee_Hit);

// 투사체 Hit(전용 GA)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_TargetHit);

// 던지기 애님 노티파이용(전용 GA)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Event_ThrowRelease);

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

// ASC 제외 공격 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Damage_Object);
