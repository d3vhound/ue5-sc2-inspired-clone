#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AetherfrontResourceNode.generated.h"

class USphereComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class AETHERFRONT_API AAetherfrontResourceNode final : public AActor
{
    GENERATED_BODY()

public:
    AAetherfrontResourceNode();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeAuthority(const FString& InResourceId, int32 InRemainingAlloy);

private:
    void ApplyVisuals();

    UPROPERTY(VisibleAnywhere, Category = "Resource")
    TObjectPtr<USphereComponent> CollisionRoot;

    UPROPERTY(VisibleAnywhere, Category = "Resource")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(Replicated)
    FString ResourceId;

    UPROPERTY(Replicated)
    int32 RemainingAlloy = 12000;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ResourceMaterial;
};
