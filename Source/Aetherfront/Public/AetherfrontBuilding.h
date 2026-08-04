#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AetherfrontTypes.h"
#include "AetherfrontBuilding.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class AETHERFRONT_API AAetherfrontBuilding final : public AActor
{
    GENERATED_BODY()

public:
    AAetherfrontBuilding();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeAuthority(
        const FString& InEntityId,
        const FString& InOwnerId,
        EAetherfrontBuildingKind InKind,
        const FLinearColor& InTeamColor,
        float InBuildProgress = 0.0f);

    void SetLocallySelected(bool bSelected);
    FAetherfrontBuildingSaveRecord ToSaveRecord() const;

    bool IsOwnedBy(const FString& PlayerId) const { return !PlayerId.IsEmpty() && OwnerId == PlayerId; }
    bool IsComplete() const { return BuildProgress >= 1.0f; }
    const FString& GetOwnerId() const { return OwnerId; }
    EAetherfrontBuildingKind GetBuildingKind() const { return BuildingKind; }
    float GetBuildProgress() const { return BuildProgress; }

    static int32 GetAlloyCost(EAetherfrontBuildingKind Kind);
    static float GetFootprintRadius(EAetherfrontBuildingKind Kind);

private:
    UFUNCTION()
    void OnRep_State();

    void ApplyVisuals();
    void ApplyConstructionVisual();

    UPROPERTY(VisibleAnywhere, Category = "Building")
    TObjectPtr<UBoxComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category = "Building")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Building")
    TObjectPtr<UStaticMeshComponent> SelectionMarker;

    UPROPERTY(ReplicatedUsing = OnRep_State)
    FString EntityId;

    UPROPERTY(ReplicatedUsing = OnRep_State)
    FString OwnerId;

    UPROPERTY(ReplicatedUsing = OnRep_State)
    EAetherfrontBuildingKind BuildingKind = EAetherfrontBuildingKind::Relay;

    UPROPERTY(ReplicatedUsing = OnRep_State)
    FLinearColor TeamColor = FLinearColor(0.12f, 0.75f, 1.0f);

    UPROPERTY(ReplicatedUsing = OnRep_State)
    float BuildProgress = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BuildingMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SelectionMaterial;

    FVector AuthoredMeshScale = FVector::OneVector;
    float BuildDurationSeconds = 9.0f;
};
