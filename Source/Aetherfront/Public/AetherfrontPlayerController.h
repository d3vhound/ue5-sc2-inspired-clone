#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AetherfrontTypes.h"
#include "AetherfrontPlayerController.generated.h"

class AAetherfrontBuilding;
class AAetherfrontHUD;
class AAetherfrontUnit;

UCLASS()
class AETHERFRONT_API AAetherfrontPlayerController final : public APlayerController
{
    GENERATED_BODY()

public:
    AAetherfrontPlayerController();

    UFUNCTION(Client, Reliable)
    void ClientInitializeCamera(FVector_NetQuantize10 Location, FRotator Rotation);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

private:
    void HandleSelectPressed();
    void HandleSelectReleased();
    void HandleCommand();
    void HandleBuildToggle();

    void ClearSelection();
    void SelectSingleActor(AActor* Actor, bool bAddToSelection);
    void SelectUnitsInRectangle(const FVector2D& Start, const FVector2D& End, bool bAddToSelection);
    void UpdateHUD();
    FString GetPersistentPlayerId() const;
    bool IsBuildingLocationValid(EAetherfrontBuildingKind Kind, const FVector& Location) const;

    UFUNCTION(Server, Reliable)
    void ServerIssueMove(
        const TArray<AAetherfrontUnit*>& Units,
        FVector_NetQuantize10 Target,
        bool bQueue,
        uint32 CommandSequence);

    UFUNCTION(Server, Reliable)
    void ServerPlaceBuilding(EAetherfrontBuildingKind Kind, FVector_NetQuantize10 Location);

    UPROPERTY(Transient)
    TArray<TObjectPtr<AAetherfrontUnit>> SelectedUnits;

    UPROPERTY(Transient)
    TObjectPtr<AAetherfrontBuilding> SelectedBuilding;

    FVector2D SelectionStart = FVector2D::ZeroVector;
    FVector2D SelectionCurrent = FVector2D::ZeroVector;
    bool bSelecting = false;
    bool bBuildMode = false;
    uint32 NextCommandSequence = 1;
    double LastMoveCommandServerTime = -1.0;
    double LastBuildCommandServerTime = -1.0;
    FVector PendingCameraLocation = FVector::ZeroVector;
    FRotator PendingCameraRotation = FRotator::ZeroRotator;
    bool bHasPendingCameraTransform = false;
};
