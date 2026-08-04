#include "AetherfrontWorldDirector.h"

#include "AetherfrontResourceNode.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    Ground->SetRelativeScale3D(FVector(20000.0f, 20000.0f, 1.0f));
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

void AAetherfrontWorldDirector::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInstanceDynamic* GroundMaterial = Ground->CreateAndSetMaterialInstanceDynamic(0))
    {
        GroundMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.055f, 0.075f, 0.072f));
    }

    if (HasAuthority())
    {
        GenerateResources();
    }
}

void AAetherfrontWorldDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontWorldDirector, WorldSeed);
}

void AAetherfrontWorldDirector::GenerateResources()
{
    FRandomStream Random(WorldSeed);
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    constexpr int32 ResourceCount = 160;
    constexpr float InnerRadius = 4500.0f;
    constexpr float OuterRadius = 65000.0f;
    for (int32 Index = 0; Index < ResourceCount; ++Index)
    {
        const float RadiusAlpha = FMath::Sqrt(Random.FRand());
        const float Radius = FMath::Lerp(InnerRadius, OuterRadius, RadiusAlpha);
        const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
        const FVector Location(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 125.0f);

        AAetherfrontResourceNode* Node = GetWorld()->SpawnActor<AAetherfrontResourceNode>(
            AAetherfrontResourceNode::StaticClass(),
            FTransform(FRotator(0.0f, Random.FRandRange(0.0f, 360.0f), 0.0f), Location),
            SpawnParameters);
        if (Node)
        {
            Node->InitializeAuthority(
                FString::Printf(TEXT("resource-%08d-%04d"), WorldSeed, Index),
                Random.RandRange(9000, 18000));
        }
    }
}
