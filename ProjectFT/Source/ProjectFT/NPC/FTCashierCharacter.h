#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "FTCashierCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTCashierCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTCashierCharacter();
	
	/** 캐셔가 기본 대기 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Cashier|State")
	bool bIsIdle = false;
	
	/** 캐셔가 범죄행위를 발견하고 놀란 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Cashier|State")
	bool bIsAlerted = false;
	
	/** 캐셔가 신고 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Cashier|State")
	bool bIsReporting = false;
	
	/** 캐셔가 신고 완료 후 대기 상태 애니메이션을 재생해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Cashier|State")
	bool bIsReportedIdle = false;

protected:
	virtual void OnDeath() override;
};
