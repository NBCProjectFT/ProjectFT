// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "FTTraversalComponent.generated.h"

class ACharacter;
class UMotionWarpingComponent;
class UAnimMontage;
class UPrimitiveComponent;

// GASP의 E_TraversalActionType와 동일한 1차 액션 분류. Climb/Catch 등 세부 분기는 두지 않는다.
UENUM(BlueprintType)
enum class EFTTraversalActionType : uint8
{
	None,
	Vault,  // 얇은 장애물을 넘어 반대편으로 낙하(반대편 바닥 없음/큰 낙차)
	Hurdle, // 얇은 장애물을 넘어 반대편 바닥에 착지(낮은 펜스)
	Mantle  // 넘지 않고 위로 올라서기(두꺼운 장애물)
};

// 트래버설 판정/연출 튜닝값. GASP의 UFTTraversalSettings 설계를 컴포넌트 인라인 구조체로 정리한 것.
// (별도 DataAsset이 필요하면 이 구조체를 그대로 UDataAsset 필드로 승격하면 된다.)
USTRUCT(BlueprintType)
struct FFTTraversalSettings
{
	GENERATED_BODY()

	// 액션 허용 토글.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") bool bAllowVault = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") bool bAllowHurdle = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") bool bAllowMantle = true;

	// 장애물 윗면까지의 높이(캐릭터 발 기준) 한계.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float MinObstacleHeight = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float MaxVaultHeight = 125.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float MaxHurdleHeight = 125.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float MaxMantleHeight = 275.0f;

	// 이 깊이 미만이면 얇은 장애물(Vault/Hurdle), 이상이면 Mantle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float ThinDepthThreshold = 59.0f;

	// Hurdle 판정: 뒤쪽 낙차(BackLedgeHeight)가 이 값보다 커야 Hurdle로 본다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal") float MinBackFloorDrop = 50.0f;

	// 몽타주가 없을 때 프로토타입 위치 보간에 쓰는 소요 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Prototype") float VaultDuration = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Prototype") float HurdleDuration = 0.60f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Prototype") float MantleDuration = 0.70f;

	// 전방/윗면 트레이스 파라미터.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace") float MinForwardTraceDistance = 75.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace") float MaxForwardTraceDistance = 350.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace") float TopScanStep = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace") float MaxScanDepth = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Trace") float TopZTolerance = 8.0f;
};

// 트레이스로 만들어낸 한 번의 트래버설 판정 결과. GASP S_TraversalCheckResult의 이름 컨벤션을 따른다.
USTRUCT(BlueprintType)
struct FFTTraversalCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") EFTTraversalActionType ActionType = EFTTraversalActionType::None;

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") bool bHasFrontLedge = false;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") FVector FrontLedgeLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") FVector FrontLedgeNormal = FVector::ZeroVector; // 수평, 장애물 앞면 바깥(캐릭터 쪽)

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") bool bHasBackLedge = false;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") FVector BackLedgeLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") FVector BackLedgeNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") bool bHasFloor = false;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") FVector FloorLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") float ObstacleHeight = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") float ObstacleDepth = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") float BackLedgeHeight = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") float DistanceToLedge = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Traversal") TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Traversal") TObjectPtr<UAnimMontage> ChosenMontage = nullptr;
};

// 트래버설을 시작하는 순간 판정 결과를 알린다(AnimBP/UI/디버그 훅용).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnTraversalStarted, const FFTTraversalCandidate&, Candidate);

/**
 * 트레이스 기반 파쿠르(Vault/Hurdle/Mantle) 컴포넌트.
 * GASP의 로직 계층(장애물 탐지 → 액션 분류 → 워프 타깃 빌드)만 이식하고,
 * 애니 선택은 모션매칭/Chooser 대신 액션별 몽타주 1개 직접 재생으로 단순화했다.
 * 정렬은 MotionWarping의 FrontLedge 타깃 1개로 처리한다(데이터셋 불필요, 몽타주에 FrontLedge 노티파이 1개만 필요).
 * 몽타주가 비어 있으면 액터 위치 보간으로 폴백해 애니 없이도 로직을 검증할 수 있다.
 * 입력은 받지 않는다 — 소유 캐릭터가 TryStartTraversal()에서 TryTraversal()을 호출한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTTraversalComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 점프 입력 등에서 호출하는 단일 진입점. 유효한 액션을 찾아 시작했으면 true(→ 일반 점프 생략).
	UFUNCTION(BlueprintCallable, Category = "FT|Traversal")
	bool TryTraversal();

	UFUNCTION(BlueprintPure, Category = "FT|Traversal")
	bool IsTraversing() const { return bIsTraversing; }

	UPROPERTY(BlueprintAssignable, Category = "FT|Traversal")
	FFTOnTraversalStarted OnTraversalStarted;

	// 액션별 재생 몽타주. 비워두면 프로토타입 위치 보간으로 폴백한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	TMap<EFTTraversalActionType, TObjectPtr<UAnimMontage>> MontagesByAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	FFTTraversalSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal|Debug")
	bool bDrawDebug = false;

	// ── 문서화된 단계별 API (BP에서 개별 호출/재정의 가능) ──

	// 전방/윗면/뒷면 트레이스로 렛지·바닥·높이·깊이를 채운다. 앞 렛지를 찾으면 true.
	UFUNCTION(BlueprintCallable, Category = "FT|Traversal")
	bool FindTraversalCandidate(FFTTraversalCandidate& OutCandidate) const;

	// 높이/깊이/바닥 조건으로 Vault/Hurdle/Mantle 중 하나(또는 None)를 정한다.
	UFUNCTION(BlueprintCallable, Category = "FT|Traversal")
	void ClassifyTraversal(UPARAM(ref) FFTTraversalCandidate& Candidate) const;

	// MotionWarping 타깃을 갱신한다. FrontLedge는 항상, BackLedge/Floor는 있을 때만.
	UFUNCTION(BlueprintCallable, Category = "FT|Traversal")
	void BuildWarpTargets(const FFTTraversalCandidate& Candidate) const;

	// 액션을 실제로 시작한다(몽타주 재생 또는 위치 보간). OnTraversalStarted를 broadcast.
	UFUNCTION(BlueprintCallable, Category = "FT|Traversal")
	void StartTraversal(const FFTTraversalCandidate& Candidate);

protected:
	virtual void BeginPlay() override;

	void PlayTraversalMontage(UAnimMontage* Montage);
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void BeginPrototypeInterp(const FFTTraversalCandidate& Candidate);
	FVector ComputeInterpTarget(const FFTTraversalCandidate& Candidate) const;
	void FinishTraversal();

	// 전방 속도에 비례해 전방 트레이스 길이를 늘린다(빠르게 달릴수록 더 멀리 탐지).
	float GetForwardTraceDistance() const;

	UPROPERTY(Transient) TObjectPtr<ACharacter> OwnerCharacter;
	UPROPERTY(Transient) TObjectPtr<UMotionWarpingComponent> MotionWarping;

	FFTTraversalCandidate ActiveCandidate;
	bool bIsTraversing = false;

	// 프로토타입(몽타주 없음) 위치 보간 상태.
	bool bInterpolating = false;
	FVector InterpStart = FVector::ZeroVector;
	FVector InterpTarget = FVector::ZeroVector;
	float InterpElapsed = 0.0f;
	float InterpDuration = 0.0f;
};
