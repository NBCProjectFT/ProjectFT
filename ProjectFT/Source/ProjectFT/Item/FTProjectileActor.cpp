// Fill out your copyright notice in the Description page of Project Settings.


#include "FTProjectileActor.h"


// Sets default values
AFTProjectileActor::AFTProjectileActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AFTProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFTProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

