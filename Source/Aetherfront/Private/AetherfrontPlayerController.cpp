#include "AetherfrontPlayerController.h"

#include "Aetherfront.h"

AAetherfrontPlayerController::AAetherfrontPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AAetherfrontPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
    }
}

void AAetherfrontPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);
    InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &AAetherfrontPlayerController::HandleSelect);
    InputComponent->BindAction(TEXT("Command"), IE_Pressed, this, &AAetherfrontPlayerController::HandleCommand);
}

void AAetherfrontPlayerController::HandleSelect()
{
    FHitResult Hit;
    if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
    {
        UE_LOG(LogAetherfront, Verbose, TEXT("Selection hit: %s"), *GetNameSafe(Hit.GetActor()));
    }
}

void AAetherfrontPlayerController::HandleCommand()
{
    FHitResult Hit;
    if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
    {
        UE_LOG(LogAetherfront, Verbose, TEXT("Command target: %s"), *Hit.ImpactPoint.ToCompactString());
    }
}
