// Fill out your copyright notice in the Description page of Project Settings.


#include "HuntersGamePlaneActor.h"


// Sets default values
AHuntersGamePlaneActor::AHuntersGamePlaneActor()
{
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	RootComponent = PlaneMesh;
}

// Called when the game starts or when spawned
void AHuntersGamePlaneActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHuntersGamePlaneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

