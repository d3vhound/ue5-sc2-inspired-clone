#include "AetherfrontUnit.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAetherfrontUnit::AAetherfrontUnit()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(15.0f);
    SetMinNetUpdateFrequency(5.0f);
    SetNetCullDistanceSquared(FMath::Square(160000.0f));

    CollisionRoot = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionRoot"));
    CollisionRoot->InitSphereRadius(52.0f);
    CollisionRoot->SetCollisionProfileName(TEXT("Pawn"));
    CollisionRoot->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    CollisionRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SetRootComponent(CollisionRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(CollisionRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -4.0f));

    SelectionMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionMarker"));
    SelectionMarker->SetupAttachment(CollisionRoot);
    SelectionMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -48.0f));
    SelectionMarker->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.035f));
    SelectionMarker->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> UnitMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (UnitMesh.Succeeded())
    {
        Mesh->SetStaticMesh(UnitMesh.Object);
    }
    if (MarkerMesh.Succeeded())
    {
        SelectionMarker->SetStaticMesh(MarkerMesh.Object);
    }

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 650.0f;
    Movement->Acceleration = 1700.0f;
    Movement->Deceleration = 2300.0f;
    Movement->TurningBoost = 6.0f;
}

void AAetherfrontUnit::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisuals();
}

void AAetherfrontUnit::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !bHasMoveTarget)
    {
        return;
    }

    FVector ToTarget = FVector(MoveTarget) - GetActorLocation();
    ToTarget.Z = 0.0f;
    const float Distance = ToTarget.Size();
    if (Distance <= AcceptanceRadius)
    {
        if (!QueuedMoveTargets.IsEmpty())
        {
            MoveTarget = QueuedMoveTargets[0];
            QueuedMoveTargets.RemoveAt(0, 1, EAllowShrinking::No);
        }
        else
        {
            bHasMoveTarget = false;
            Movement->StopMovementImmediately();
        }
        ForceNetUpdate();
        return;
    }

    const FVector Direction = ToTarget / Distance;
    Movement->AddInputVector(Direction, true);

    const FRotator DesiredRotation = Direction.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 9.0f));
}

void AAetherfrontUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontUnit, EntityId);
    DOREPLIFETIME(AAetherfrontUnit, OwnerId);
    DOREPLIFETIME(AAetherfrontUnit, UnitKind);
    DOREPLIFETIME(AAetherfrontUnit, TeamColor);
    DOREPLIFETIME(AAetherfrontUnit, MoveTarget);
    DOREPLIFETIME(AAetherfrontUnit, bHasMoveTarget);
    DOREPLIFETIME(AAetherfrontUnit, LastCommandSequence);
}

void AAetherfrontUnit::InitializeAuthority(
    const FString& InEntityId,
    const FString& InOwnerId,
    const EAetherfrontUnitKind InKind,
    const FLinearColor& InTeamColor)
{
    check(HasAuthority());
    EntityId = InEntityId;
    OwnerId = InOwnerId;
    UnitKind = InKind;
    TeamColor = InTeamColor;
    ApplyVisuals();
    ForceNetUpdate();
}

void AAetherfrontUnit::SetMoveTargetAuthority(
    const FVector& InTarget,
    const uint32 InCommandSequence,
    const bool bQueue)
{
    check(HasAuthority());

    FVector SanitizedTarget = InTarget;
    constexpr float WorldHalfExtent = 900000.0f;
    SanitizedTarget.X = FMath::Clamp(SanitizedTarget.X, -WorldHalfExtent, WorldHalfExtent);
    SanitizedTarget.Y = FMath::Clamp(SanitizedTarget.Y, -WorldHalfExtent, WorldHalfExtent);
    SanitizedTarget.Z = GetActorLocation().Z;
    if (bQueue && bHasMoveTarget)
    {
        constexpr int32 MaxQueuedCommands = 16;
        if (QueuedMoveTargets.Num() < MaxQueuedCommands)
        {
            QueuedMoveTargets.Add(SanitizedTarget);
        }
    }
    else
    {
        QueuedMoveTargets.Reset();
        MoveTarget = SanitizedTarget;
        bHasMoveTarget = true;
    }

    LastCommandSequence = InCommandSequence;
    ForceNetUpdate();
}

void AAetherfrontUnit::SetLocallySelected(const bool bSelected)
{
    SelectionMarker->SetVisibility(bSelected, true);
}

void AAetherfrontUnit::OnRep_Identity()
{
    ApplyVisuals();
}

void AAetherfrontUnit::ApplyVisuals()
{
    const FVector MeshScale = UnitKind == EAetherfrontUnitKind::Fabricator
        ? FVector(0.72f, 0.72f, 0.62f)
        : FVector(0.88f, 0.72f, 0.92f);
    Mesh->SetRelativeScale3D(MeshScale);

    if (!UnitMaterial)
    {
        UnitMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (UnitMaterial)
    {
        const FLinearColor KindTint = UnitKind == EAetherfrontUnitKind::Fabricator
            ? TeamColor
            : FLinearColor::LerpUsingHSV(TeamColor, FLinearColor::White, 0.28f);
        UnitMaterial->SetVectorParameterValue(TEXT("Color"), KindTint);
    }

    if (!SelectionMaterial)
    {
        SelectionMaterial = SelectionMarker->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (SelectionMaterial)
    {
        SelectionMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.95f, 0.78f));
    }
}
