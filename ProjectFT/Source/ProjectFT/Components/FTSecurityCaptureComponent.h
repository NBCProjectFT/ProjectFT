#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityCaptureComponent.generated.h"

/**
 * 보안요원의 플레이어 체포 소유권을 관리하는 컴포넌트다.
 * 최초 체포 요청자만 Captor로 승인하고 체포 및 탈출 Gameplay Message를 발행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCaptureComponent();

	/** 최초 체포 요청자를 Captor로 등록하고 TargetCaptured 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Capture")
	bool TryCaptureTarget(AActor* SecurityActor, AActor* TargetActor, FVector CaptureLocation);

	/** 붙잡힌 대상의 탈출 요청을 검증하고 TargetEscaped 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Capture")
	bool RequestTargetEscape(AActor* TargetActor, FVector EscapeLocation);

	/** 유효한 체포 상태가 존재하는지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Capture")
	bool IsTargetCaptured() const;

	/** 현재 플레이어를 붙잡고 있는 보안요원을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Capture")
	AActor* GetCaptorActor() const;

	/** 현재 붙잡힌 대상을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Capture")
	AActor* GetCapturedTargetActor() const;

private:
	/** 현재 체포 소유권을 가진 보안요원이다. */
	TWeakObjectPtr<AActor> CaptorActor;

	/** 현재 붙잡힌 대상이다. */
	TWeakObjectPtr<AActor> CapturedTargetActor;
};
