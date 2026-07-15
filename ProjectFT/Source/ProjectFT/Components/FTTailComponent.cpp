#include "FTTailComponent.h"

#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"

UFTTailComponent::UFTTailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	bReceivesDecals = false;

	TailAnimatedBoneNames =
	{
		TEXT("RigTail1"),
		TEXT("RigTail2"),
		TEXT("RigTail3"),
		TEXT("RigTail4"),
		TEXT("RigTail5")
	};

}

void UFTTailComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeTailMesh();
}

void UFTTailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTailPose();
}

void UFTTailComponent::InitializeTailMesh()
{
	if (TailSkeletalMesh)
	{
		SetSkinnedAssetAndUpdate(TailSkeletalMesh);
	}
}

void UFTTailComponent::UpdateTailPose()
{
	if (TailAnimatedBoneNames.IsEmpty())
	{
		return;
	}

	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	const float Time = World ? World->GetTimeSeconds() : 0.0f;
	const float SpeedAlpha = Owner ? FMath::Clamp(Owner->GetVelocity().Size() / 600.0f, 0.0f, 1.0f) : 0.0f;
	const float MovementBoost = 1.0f + SpeedAlpha * TailMovementInfluence;
	const float BoneCount = static_cast<float>(TailAnimatedBoneNames.Num());

	for (int32 BoneIndex = 0; BoneIndex < TailAnimatedBoneNames.Num(); ++BoneIndex)
	{
		const FName BoneName = TailAnimatedBoneNames[BoneIndex];
		if (GetBoneIndex(BoneName) == INDEX_NONE)
		{
			continue;
		}

		const float ChainAlpha = (static_cast<float>(BoneIndex) + 1.0f) / BoneCount;
		const float WavePhase = FMath::Frac(Time * TailWaveSpeed - ChainAlpha * TailPhaseOffsetPerBone);
		const float WaveValue = EvaluateCurve(WaveCurve, WavePhase, 0.0f);
		const float AmplitudeValue = EvaluateCurve(AmplitudeCurve, ChainAlpha, 1.0f);
		const float StandRoll = EvaluateCurve(StandCurve, ChainAlpha, 0.0f);
		const float Yaw = WaveValue * TailWaveAmplitude * AmplitudeValue * MovementBoost;
		const float Pitch = WaveValue * TailWaveAmplitude * TailPitchMultiplier * AmplitudeValue;

		SetBoneRotationByName(BoneName, FRotator(Pitch, Yaw, StandRoll), EBoneSpaces::ComponentSpace);
	}
}

float UFTTailComponent::EvaluateCurve(const UCurveFloat* Curve, float Input, float Fallback) const
{
	return Curve && Curve->FloatCurve.GetNumKeys() > 0 ? Curve->GetFloatValue(Input) : Fallback;
}
