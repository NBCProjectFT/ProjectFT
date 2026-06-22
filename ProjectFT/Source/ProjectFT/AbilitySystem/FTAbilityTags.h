#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 아이템 사용 어빌리티의 쿨다운 식별 태그. 쿨다운 GE가 적용되는 동안 소유자에게 부여되며, 어빌리티의 CheckCooldown이 이 태그로 재사용을 차단한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Cooldown_ItemUse);

// 쿨다운 지속시간을 SetByCaller로 주입할 때 쓰는 데이터 태그(어빌리티별 CooldownSeconds 주입 키).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Data_Cooldown);

// 아이템 사용 어빌리티가 시전(활성) 중인 동안 소유자에게 부여되는 상태 태그. 이동 입력 시 이 태그로 시전을 취소한다.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_State_UsingItem);

// Quick-slot input tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Input_QuickSlot_1);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Input_QuickSlot_2);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Input_QuickSlot_3);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Input_QuickSlot_4);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Input_Item_Primary);

// Item behavior tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Action_Heal);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Action_Throw);

// Item classification tags. ItemDataAsset may own multiple types at once.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Misc);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Usable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Material);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Weapon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Armor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Consumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_RecipeBook);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FT_Item_Type_Quest);
