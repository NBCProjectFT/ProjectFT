#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FTNPCReportComponent.generated.h"

class AFTNPCAIController;
class UAnimMontage;
struct FFTCharacterAttackedPayloadStruct;
struct FFTMessagePayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTNPCReportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTNPCReportComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportDecayDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportCancelDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report|Animation")
	TObjectPtr<UAnimMontage> ReportMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report|Animation", meta = (ClampMin = "0.0"))
	float ReportMontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report|Animation", meta = (ClampMin = "0.0"))
	float ReportMontageBlendOutTime = 0.15f;

	/** 신고 완료 후 같은 손님이 다시 신고할 수 있기까지 기다리는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report", meta = (ClampMin = "0.0"))
	float ReportCooldown = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float CurrentReportProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCancelled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedShelfDamaged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedAssault = false;

	void EnterReporting();
	bool TickReporting(float DeltaTime);
	void CancelReport();
	void HandleReportFlowAvailability(bool bCanStartReportFlow, bool bLogReportDebug);
	bool HandleShelfDamaged(const FFTMessagePayloadStruct& Payload);
	bool HandleObservedAssault(const FFTCharacterAttackedPayloadStruct& Payload);
	void MarkObservedShelfDamage();
	void MarkObservedAssault();
	void HandleStunStateChanged(bool bStunned);
	bool CanStartReport() const;

private:
	float ReportElapsedTime = 0.0f;
	float LastReportCompletedTime = -FLT_MAX;
	int32 LastLoggedReportPercent = -1;
	int32 LastLoggedReportDecayPercent = 101;

	AFTNPCAIController* GetNPCAIController() const;
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool ShouldCancelReport(const AFTNPCAIController* Controller) const;
	void PlayReportMontage() const;
	void StopReportMontage() const;
	void CompleteReport();
	void BroadcastReportMessage(FGameplayTag Channel, AActor* TargetActor, float InReportAmount, float ReportProgress) const;
	void ResetReportState();
	bool IsReportCooldownReady() const;
};
