#include "FTTestComponent.h"
#include "../Struct/FTTestStruct.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

// UE_DEFINE_GAMEPLAY_TAG_STATIC(TestTag, "Tests.GenericTag");

UFTTestComponent::UFTTestComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}
void UFTTestComponent::BeginPlay()
{
	Super::BeginPlay();	
}

void UFTTestComponent::OnComponentDataChanged()
{
	// Broadcast 하는 코드
	FTTestStruct TestStructure;
	TestStructure.data = data;
	// UGameplayMessageSubsystem::Get(this).BroadcastMessage(TestTag, TestStructure);	
}
