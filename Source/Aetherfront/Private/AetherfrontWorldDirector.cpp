#include "AetherfrontWorldDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAetherfrontWorldDirector::AAetherfrontWorldDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
    Ground->SetupAttachment(SceneRoot);
    Ground->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
    Ground->SetRelativeScale3D(FVector(200.0f, 200.0f, 1.0f));
    Ground->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Ground->SetMobility(EComponentMobility::Static);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (GroundMesh.Succeeded())
    {
        Ground->SetStaticMesh(GroundMesh.Object);
    }

    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(SceneRoot);
    Sun->SetRelativeRotation(FRotator(-48.0f, -28.0f, 0.0f));
    Sun->SetIntensity(7.5f);
    Sun->SetLightColor(FLinearColor(1.0f, 0.82f, 0.65f));
    Sun->bAtmosphereSunLight = true;

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetIntensity(0.75f);
    SkyLight->bRealTimeCapture = true;

    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(SceneRoot);
    Fog->SetFogDensity(0.0015f);
    Fog->SetFogHeightFalloff(0.14f);
    Fog->SetFogInscatteringColor(FLinearColor(0.08f, 0.12f, 0.16f));

    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Atmosphere->SetupAttachment(SceneRoot);
}

void AAetherfrontWorldDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontWorldDirector, WorldSeed);
}
