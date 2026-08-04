#pragma once

#include "CoreMinimal.h"
#include "AetherfrontTypes.generated.h"

UENUM(BlueprintType)
enum class EAetherfrontUnitKind : uint8
{
    Fabricator,
    Warden
};

UENUM(BlueprintType)
enum class EAetherfrontBuildingKind : uint8
{
    Citadel,
    Relay,
    Extractor
};

USTRUCT(BlueprintType)
struct FAetherfrontBuildingSaveRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString EntityId;

    UPROPERTY()
    FString OwnerId;

    UPROPERTY()
    EAetherfrontBuildingKind Kind = EAetherfrontBuildingKind::Relay;

    UPROPERTY()
    FTransform Transform = FTransform::Identity;

    UPROPERTY()
    float BuildProgress = 1.0f;
};

USTRUCT(BlueprintType)
struct FAetherfrontWorldSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 SchemaVersion = 1;

    UPROPERTY()
    int32 WorldSeed = 20260804;

    UPROPERTY()
    int64 AuthoritativeTick = 0;

    UPROPERTY()
    TArray<FAetherfrontBuildingSaveRecord> Buildings;
};
