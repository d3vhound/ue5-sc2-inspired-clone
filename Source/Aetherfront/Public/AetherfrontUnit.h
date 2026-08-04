#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AetherfrontTypes.h"
#include "AetherfrontUnit.generated.h"

class UFloatingPawnMovement;
class UMaterialInstanceDynamic;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class AETHERFRONT_API AAetherfrontUnit final : public APawn
{
    GENERATED_BODY()

public:
    AAetherfrontUnit();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeAuthority(
        const FString& InEntityId,
        const FString& InOwnerId,
        EAetherfrontUnitKind InKind,
        const FLinearColor& InTeamColor);

    void SetMoveTargetAuthority(const FVector& InTarget, uint32 InCommandSequence, bool bQueue);
    void SetLocallySelected(bool bSelected);

    bool IsOwnedBy(const FString& PlayerId) const { return !PlayerId.IsEmpty() && OwnerId == PlayerId; }
    const FString& GetOwnerId() const { return OwnerId; }
    const FString& GetEntityId() const { return EntityId; }

private:
    UFUNCTION()
    void OnRep_Identity();

    void ApplyVisuals();

    UPROPERTY(VisibleAnywhere, Category = "Unit")
    TObjectPtr<USphereComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category = "Unit")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Unit")
    TObjectPtr<UStaticMeshComponent> SelectionMarker;

    UPROPERTY(VisibleAnywhere, Category = "Movement")
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(ReplicatedUsing = OnRep_Identity)
    FString EntityId;

    UPROPERTY(ReplicatedUsing = OnRep_Identity)
    FString OwnerId;

    UPROPERTY(ReplicatedUsing = OnRep_Identity)
    EAetherfrontUnitKind UnitKind = EAetherfrontUnitKind::Fabricator;

    UPROPERTY(ReplicatedUsing = OnRep_Identity)
    FLinearColor TeamColor = FLinearColor(0.12f, 0.75f, 1.0f);

    UPROPERTY(Replicated)
    FVector_NetQuantize10 MoveTarget = FVector::ZeroVector;

    UPROPERTY(Replicated)
    bool bHasMoveTarget = false;

    UPROPERTY(Replicated)
    uint32 LastCommandSequence = 0;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> UnitMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SelectionMaterial;

    TArray<FVector> QueuedMoveTargets;

    float AcceptanceRadius = 55.0f;
};
