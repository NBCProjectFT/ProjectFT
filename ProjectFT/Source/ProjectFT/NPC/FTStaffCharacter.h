#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/NPC/FTCashierCharacter.h"
#include "FTStaffCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTStaffCharacter : public AFTCashierCharacter
{
	GENERATED_BODY()

public:
	AFTStaffCharacter();

	/** 직원이 매대나 대기 지점을 확인하는 애니메이션 상태다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Staff|State")
	bool bIsCheckingShelf = false;

	/** 직원이 매대를 재보충하는 애니메이션 상태다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Staff|State")
	bool bIsRestocking = false;
};
