// Fill out your copyright notice in the Description page of Project Settings.


#include "TestProcessor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"
#include "../Struct/FTTestStruct.h"

// TAG 생성
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestTag, "Tests.GenericTag");

void UTestProcessor::StartListening()
{
	// Listener 등록하는 코드
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(TestTag, this, &ThisClass::OnTestMessage));
}
void UTestProcessor::StopListening()
{
	Super::StopListening();
	
	
}

// Listener로 함수 호출 됨
void UTestProcessor::OnTestMessage(FGameplayTag Channel, const FTTestStruct& Payload)
{
	FTTestStruct TestStructure;
	TestStructure.data = Payload.data;
}
