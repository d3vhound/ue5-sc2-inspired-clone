#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AetherfrontCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class USphereComponent;
class USpringArmComponent;

UCLASS()
class AETHERFRONT_API AAetherfrontCameraPawn final : public APawn
{
    GENERATED_BODY()

public:
    AAetherfrontCameraPawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
    void RotateLeft();
    void RotateRight();

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<USphereComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, Category = "Movement")
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "500.0"))
    float MinZoom = 900.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "1000.0"))
    float MaxZoom = 5200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float ZoomStep = 320.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float RotationStepDegrees = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float WorldExtent = 950000.0f;
};

