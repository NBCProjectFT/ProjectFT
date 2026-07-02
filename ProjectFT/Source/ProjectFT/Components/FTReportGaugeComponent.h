#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "UObject/ObjectKey.h"
#include "FTReportGaugeComponent.generated.h"

struct FFTNPCReportPayloadStruct;

/**
 * 손님 NPC의 신고 게이지를 관리하는 컴포넌트이다.
 *
 * Reporter별 신고 게이지를 관리하고 신고 관련 Gameplay Message를 처리한다.
 * 각 Reporter의 게이지가 최대치에 도달하면 보안 호출 메시지를 한 번 발행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTReportGaugeComponent : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	UFTReportGaugeComponent();

	/** Payload에 포함된 Reporter의 신고 게이지에 ReportAmount를 누적한다. */
	void AddReportGauge(const FFTNPCReportPayloadStruct& Payload);

	/** 모든 Reporter의 신고 게이지와 보안 호출 기록을 초기화한다. */
	void ResetAllReportGauges();

	/** 현재 Reporter 게이지 중 가장 높은 비율을 반환한다. */
	float GetHighestReportGaugeRatio() const;

	/** 지정한 Reporter의 신고 게이지 비율을 반환한다. */
	float GetReportGaugeRatio(AActor* ReportActor) const;

protected:
	/** NPC 신고 메시지를 수신하는 Gameplay Message 리스너를 등록한다. */
	virtual void StartListening() override;

	/** Reporter 한 명이 보안 호출을 발생시키는 최대 신고 게이지다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|ReportGauge")
	float MaxReportGauge = 100.0f;

private:
	/** Reporter별 현재 신고 게이지다. */
	TMap<TObjectKey<AActor>, float> ReportGaugeByReporter;

	/** 현재 신고 주기에서 이미 보안 호출을 발행한 Reporter 집합이다. */
	TSet<TObjectKey<AActor>> SecurityCalledReporters;

	/** 신고 시작 시 Reporter의 게이지와 보안 호출 기록을 초기화한다. */
	void OnReportStarted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);

	/** 신고 진행도에 맞춰 Reporter의 게이지를 갱신한다. */
	void OnReportProgress(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);

	/** 신고 완료 시 Reporter의 게이지를 최대치로 설정한다. */
	void OnReportCompleted(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);

	/** Reporter의 게이지를 유효 범위로 설정하고 변경 메시지를 발행한다. */
	void SetReporterGauge(const FFTNPCReportPayloadStruct& Payload, float NewReportGauge);

	/** Reporter의 게이지와 보안 호출 기록을 제거한다. */
	void ClearReporterGauge(AActor* ReportActor);

	/** Reporter의 게이지 변경과 필요한 보안 호출 메시지를 발행한다. */
	void BroadcastReporterGaugeChanged(const FFTNPCReportPayloadStruct& Payload);
};
