#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FTThrowReleaseAnimNotify.generated.h"

/**
 * 투척 몽타주에서 실제로 손을 놓는 프레임에 배치하는 AnimNotify다.
 *
 * Notify가 실행되면 Owner Actor의 ASC로 Event.ThrowRelease를 보내고,
 * UFTGA_ThrowItemAction은 이 이벤트를 받아 손에 들고 있던 ProjectileActor를 발사한다.
 */
UCLASS()
class PROJECTFT_API UFTThrowReleaseAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 몽타주 타임라인의 이 프레임에서 ThrowRelease GameplayEvent를 전송한다.
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;
};
