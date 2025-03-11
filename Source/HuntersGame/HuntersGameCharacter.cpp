// Copyright Epic Games, Inc. All Rights Reserved.

#include "HuntersGameCharacter.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

AHuntersGameCharacter::AHuntersGameCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;	
}

void AHuntersGameCharacter::BeginPlay()
{
	Super::BeginPlay();	
	UHeadMountedDisplayFunctionLibrary::EnableHMD(true);
}

void AHuntersGameCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
	
	// Get device rotation
	FRotator DeviceRotation = GetDeviceOrientation();
    
	// Apply rotation to character (Only Yaw is usually needed)
	FRotator NewRotation = FRotator(0.0f, DeviceRotation.Yaw, 0.0f);
    
	SetActorRotation(NewRotation);
}

FRotator AHuntersGameCharacter::GetDeviceOrientation()
{
	FRotator DeviceRotation;
	FVector DevicePosition;

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);
		// GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, FString::Printf(TEXT("Device Rotation: Pitch: %f, Roll: %f, Yaw: %f"), DeviceRotation.Pitch, DeviceRotation.Roll, DeviceRotation.Yaw));
	}
	else
	{
		// GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, FString::Printf(TEXT("HeadMountedDisplay Disabled")));
	}

	return DeviceRotation;
}
