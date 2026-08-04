#include "AetherfrontCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"

AAetherfrontCameraPawn::AAetherfrontCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bOnlyRelevantToOwner = true;
    SetReplicateMovement(false);

    CollisionRoot = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionRoot"));
    CollisionRoot->InitSphereRadius(32.0f);
    CollisionRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetRootComponent(CollisionRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(CollisionRoot);
    SpringArm->TargetArmLength = 2600.0f;
    SpringArm->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 12.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->FieldOfView = 50.0f;

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 3200.0f;
    Movement->Acceleration = 12000.0f;
    Movement->Deceleration = 16000.0f;
}

void AAetherfrontCameraPawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FVector Location = GetActorLocation();
    Location.X = FMath::Clamp(Location.X, -WorldExtent, WorldExtent);
    Location.Y = FMath::Clamp(Location.Y, -WorldExtent, WorldExtent);
    Location.Z = 100.0f;
    SetActorLocation(Location);
}

void AAetherfrontCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAetherfrontCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAetherfrontCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Zoom"), this, &AAetherfrontCameraPawn::Zoom);
    PlayerInputComponent->BindAction(TEXT("RotateLeft"), IE_Pressed, this, &AAetherfrontCameraPawn::RotateLeft);
    PlayerInputComponent->BindAction(TEXT("RotateRight"), IE_Pressed, this, &AAetherfrontCameraPawn::RotateRight);
}

void AAetherfrontCameraPawn::MoveForward(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AAetherfrontCameraPawn::MoveRight(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AAetherfrontCameraPawn::Zoom(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        SpringArm->TargetArmLength = FMath::Clamp(
            SpringArm->TargetArmLength - Value * ZoomStep,
            MinZoom,
            MaxZoom);
    }
}

void AAetherfrontCameraPawn::RotateLeft()
{
    AddActorWorldRotation(FRotator(0.0f, -RotationStepDegrees, 0.0f));
}

void AAetherfrontCameraPawn::RotateRight()
{
    AddActorWorldRotation(FRotator(0.0f, RotationStepDegrees, 0.0f));
}
