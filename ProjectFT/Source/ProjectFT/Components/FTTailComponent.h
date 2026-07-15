#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "FTTailComponent.generated.h"

class UCurveFloat;
class USkeletalMesh;

UCLASS(ClassGroup=(FT), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTTailComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UFTTailComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Tail", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMesh> TailSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FT|Tail", meta = (AllowPrivateAccess = "true"))
	TArray<FName> TailAnimatedBoneNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float TailWaveSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float TailWaveAmplitude = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float TailMovementInfluence = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (AllowPrivateAccess = "true"))
	float TailPhaseOffsetPerBone = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (AllowPrivateAccess = "true"))
	float TailPitchMultiplier = 0.25f;

	// X=wave phase(0..1), Y=side bend strength(-1..1). Use a CurveFloat asset made in the editor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> WaveCurve;

	// X=tail chain alpha(root 0..tip 1), Y=amplitude multiplier. Use a CurveFloat asset made in the editor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> AmplitudeCurve;

	// X=tail chain alpha(root 0..tip 1), Y=component-space roll angle. Use this to make the tail stand/curl.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Tail|Motion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> StandCurve;

private:
	void InitializeTailMesh();
	void UpdateTailPose();
	float EvaluateCurve(const UCurveFloat* Curve, float Input, float Fallback) const;
};
