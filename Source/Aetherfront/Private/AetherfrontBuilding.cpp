#include "AetherfrontBuilding.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAetherfrontBuilding::AAetherfrontBuilding()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    NetUpdateFrequency = 5.0f;
    NetCullDistanceSquared = FMath::Square(240000.0f);

    CollisionRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionRoot"));
    CollisionRoot->SetBoxExtent(FVector(140.0f, 140.0f, 100.0f));
    CollisionRoot->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    CollisionRoot->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SetRootComponent(CollisionRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(CollisionRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SelectionMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionMarker"));
    SelectionMarker->SetupAttachment(CollisionRoot);
    SelectionMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -98.0f));
    SelectionMarker->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildingMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (BuildingMesh.Succeeded())
    {
        Mesh->SetStaticMesh(BuildingMesh.Object);
        SelectionMarker->SetStaticMesh(BuildingMesh.Object);
    }
}

void AAetherfrontBuilding::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisuals();
}

void AAetherfrontBuilding::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || BuildProgress >= 1.0f)
    {
        return;
    }

    BuildProgress = FMath::Min(1.0f, BuildProgress + DeltaSeconds / BuildDurationSeconds);
    ApplyConstructionVisual();

    if (BuildProgress >= 1.0f)
    {
        ForceNetUpdate();
    }
}

void AAetherfrontBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontBuilding, EntityId);
    DOREPLIFETIME(AAetherfrontBuilding, OwnerId);
    DOREPLIFETIME(AAetherfrontBuilding, BuildingKind);
    DOREPLIFETIME(AAetherfrontBuilding, TeamColor);
    DOREPLIFETIME(AAetherfrontBuilding, BuildProgress);
}

void AAetherfrontBuilding::InitializeAuthority(
    const FString& InEntityId,
    const FString& InOwnerId,
    const EAetherfrontBuildingKind InKind,
    const FLinearColor& InTeamColor,
    const float InBuildProgress)
{
    check(HasAuthority());
    EntityId = InEntityId;
    OwnerId = InOwnerId;
    BuildingKind = InKind;
    TeamColor = InTeamColor;
    BuildProgress = FMath::Clamp(InBuildProgress, 0.0f, 1.0f);
    ApplyVisuals();
    ForceNetUpdate();
}

void AAetherfrontBuilding::SetLocallySelected(const bool bSelected)
{
    SelectionMarker->SetVisibility(bSelected, true);
}

FAetherfrontBuildingSaveRecord AAetherfrontBuilding::ToSaveRecord() const
{
    FAetherfrontBuildingSaveRecord Record;
    Record.EntityId = EntityId;
    Record.OwnerId = OwnerId;
    Record.Kind = BuildingKind;
    Record.Transform = GetActorTransform();
    Record.BuildProgress = BuildProgress;
    return Record;
}

int32 AAetherfrontBuilding::GetAlloyCost(const EAetherfrontBuildingKind Kind)
{
    switch (Kind)
    {
    case EAetherfrontBuildingKind::Citadel:
        return 800;
    case EAetherfrontBuildingKind::Extractor:
        return 350;
    case EAetherfrontBuildingKind::Relay:
    default:
        return 250;
    }
}

float AAetherfrontBuilding::GetFootprintRadius(const EAetherfrontBuildingKind Kind)
{
    return Kind == EAetherfrontBuildingKind::Citadel ? 310.0f : 190.0f;
}

void AAetherfrontBuilding::OnRep_State()
{
    ApplyVisuals();
}

void AAetherfrontBuilding::ApplyVisuals()
{
    switch (BuildingKind)
    {
    case EAetherfrontBuildingKind::Citadel:
        AuthoredMeshScale = FVector(4.2f, 4.2f, 2.2f);
        CollisionRoot->SetBoxExtent(FVector(260.0f, 260.0f, 125.0f));
        break;
    case EAetherfrontBuildingKind::Extractor:
        AuthoredMeshScale = FVector(2.4f, 2.4f, 2.8f);
        CollisionRoot->SetBoxExtent(FVector(170.0f, 170.0f, 140.0f));
        break;
    case EAetherfrontBuildingKind::Relay:
    default:
        AuthoredMeshScale = FVector(2.8f, 2.8f, 1.35f);
        CollisionRoot->SetBoxExtent(FVector(180.0f, 180.0f, 90.0f));
        break;
    }

    const float MarkerRadius = GetFootprintRadius(BuildingKind) / 50.0f;
    SelectionMarker->SetRelativeScale3D(FVector(MarkerRadius, MarkerRadius, 0.035f));
    SelectionMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -CollisionRoot->GetUnscaledBoxExtent().Z + 2.0f));

    if (!BuildingMaterial)
    {
        BuildingMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (BuildingMaterial)
    {
        BuildingMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
    }

    if (!SelectionMaterial)
    {
        SelectionMaterial = SelectionMarker->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (SelectionMaterial)
    {
        SelectionMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.95f, 0.78f));
    }

    ApplyConstructionVisual();
}

void AAetherfrontBuilding::ApplyConstructionVisual()
{
    FVector VisualScale = AuthoredMeshScale;
    VisualScale.Z *= FMath::Lerp(0.08f, 1.0f, FMath::SmoothStep(0.0f, 1.0f, BuildProgress));
    Mesh->SetRelativeScale3D(VisualScale);

    if (BuildingMaterial)
    {
        const FLinearColor ConstructionColor = FLinearColor::LerpUsingHSV(
            FLinearColor(0.08f, 0.10f, 0.12f),
            TeamColor,
            BuildProgress);
        BuildingMaterial->SetVectorParameterValue(TEXT("Color"), ConstructionColor);
    }
}
