#include "AetherfrontResourceNode.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAetherfrontResourceNode::AAetherfrontResourceNode()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);
    NetCullDistanceSquared = FMath::Square(140000.0f);

    CollisionRoot = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionRoot"));
    CollisionRoot->InitSphereRadius(125.0f);
    CollisionRoot->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    CollisionRoot->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SetRootComponent(CollisionRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(CollisionRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRelativeScale3D(FVector(1.15f, 1.15f, 2.5f));
    Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 180.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ResourceMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ResourceMesh.Succeeded())
    {
        Mesh->SetStaticMesh(ResourceMesh.Object);
    }
}

void AAetherfrontResourceNode::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisuals();
}

void AAetherfrontResourceNode::InitializeAuthority(const FString& InResourceId, const int32 InRemainingAlloy)
{
    check(HasAuthority());
    ResourceId = InResourceId;
    RemainingAlloy = FMath::Max(0, InRemainingAlloy);

    ApplyVisuals();
}

void AAetherfrontResourceNode::ApplyVisuals()
{
    if (!ResourceMaterial)
    {
        ResourceMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (ResourceMaterial)
    {
        ResourceMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.82f, 0.72f));
    }
}

void AAetherfrontResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontResourceNode, ResourceId);
    DOREPLIFETIME(AAetherfrontResourceNode, RemainingAlloy);
}
