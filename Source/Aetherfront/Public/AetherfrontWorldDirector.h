#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AetherfrontWorldDirector.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMeshComponent;

UCLASS()
class AETHERFRONT_API AAetherfrontWorldDirector final : public AActor
{
    GENERATED_BODY()

public:
    AAetherfrontWorldDirector();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "World")
    int32 GetWorldSeed() const { return WorldSeed; }

private:
    void GenerateResources();

    UPROPERTY(VisibleAnywhere, Category = "World")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "World")
    TObjectPtr<UStaticMeshComponent> Ground;

    UPROPERTY(VisibleAnywhere, Category = "Lighting")
    TObjectPtr<UDirectionalLightComponent> Sun;

    UPROPERTY(VisibleAnywhere, Category = "Lighting")
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere, Category = "Lighting")
    TObjectPtr<UExponentialHeightFogComponent> Fog;

    UPROPERTY(VisibleAnywhere, Category = "Lighting")
    TObjectPtr<USkyAtmosphereComponent> Atmosphere;

    UPROPERTY(EditAnywhere, Replicated, Category = "World")
    int32 WorldSeed = 20260804;
};
