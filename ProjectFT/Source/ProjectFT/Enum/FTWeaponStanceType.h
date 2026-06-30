#pragma once

#include "CoreMinimal.h"
#include "FTWeaponStanceType.generated.h"

// 손에 든 아이템에 따라 AnimBP가 고를 Idle/로코모션 스탠스.
// 전투 아키타입(Melee/HitScan/Projectile)과 분리되어 있어, 같은 공격 방식이라도
// 무기마다 다른 포즈를 줄 수 있다(예: 배트 vs 단검). 무기가 아니거나 미지정이면 Unarmed.
// 항목은 자유롭게 추가/세분화해도 된다(Blend Poses by Enum 분기와 1:1 대응).
UENUM(BlueprintType)
enum class EFTWeaponStanceType : uint8
{
	Unarmed     UMETA(DisplayName = "Unarmed"),
	Bat       UMETA(DisplayName = "Bat"),
	Pistol      UMETA(DisplayName = "Pistol"),
	Ball   UMETA(DisplayName = "Ball")
};
