#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FTGrabCaptureWindowNotifyState.generated.h"

/**
 * Grab Montage에서 실제 캡처 판정이 가능한 프레임 구간을 표시한다.
 *
 * NotifyBegin에서 활성 UFTGA_Grab 인스턴스의 캡처 창을 열고,
 * NotifyEnd에서 창을 닫는다. 캡처 성공 여부와 overlap 판정은 Ability가 소유한다.
 */
UCLASS(meta = (DisplayName = "FT Grab Capture Window"))
class PROJECTFT_API UFTGrabCaptureWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	void ForEachActiveGrabAbility(
		const USkeletalMeshComponent* MeshComp,
		TFunctionRef<void(class UFTGA_Grab&)> Callback) const;
};
