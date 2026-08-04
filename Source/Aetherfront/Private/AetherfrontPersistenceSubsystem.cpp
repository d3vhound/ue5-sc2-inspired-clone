#include "AetherfrontPersistenceSubsystem.h"

#include "Aetherfront.h"
#include "AetherfrontBuilding.h"
#include "AetherfrontTypes.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UAetherfrontPersistenceSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld(); World && World->GetNetMode() != NM_Client)
    {
        SaveWorld(World);
    }

    Super::Deinitialize();
}

void UAetherfrontPersistenceSubsystem::LoadWorld(UWorld* World)
{
    if (!World || World->GetNetMode() == NM_Client || bLoaded)
    {
        return;
    }

    bLoaded = true;
    const FString SavePath = GetSavePath();
    if (!FPaths::FileExists(SavePath))
    {
        UE_LOG(LogAetherfront, Display, TEXT("No persisted shard found; starting a new frontier."));
        return;
    }

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *SavePath))
    {
        UE_LOG(LogAetherfront, Error, TEXT("Could not read shard save: %s"), *SavePath);
        return;
    }

    FAetherfrontWorldSaveData SaveData;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &SaveData, 0, 0))
    {
        UE_LOG(LogAetherfront, Error, TEXT("Shard save is invalid JSON or has an incompatible schema."));
        return;
    }

    if (SaveData.SchemaVersion != 1)
    {
        UE_LOG(LogAetherfront, Error, TEXT("Unsupported shard schema version: %d"), SaveData.SchemaVersion);
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (const FAetherfrontBuildingSaveRecord& Record : SaveData.Buildings)
    {
        AAetherfrontBuilding* Building = World->SpawnActor<AAetherfrontBuilding>(
            AAetherfrontBuilding::StaticClass(),
            Record.Transform,
            SpawnParameters);
        if (Building)
        {
            const uint32 Hash = GetTypeHash(Record.OwnerId);
            const FLinearColor TeamColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash % 255), 185, 255);
            Building->InitializeAuthority(
                Record.EntityId,
                Record.OwnerId,
                Record.Kind,
                TeamColor,
                Record.BuildProgress);
        }
    }

    bDirty = false;
    UE_LOG(LogAetherfront, Display, TEXT("Loaded %d persisted buildings."), SaveData.Buildings.Num());
}

void UAetherfrontPersistenceSubsystem::SaveWorld(UWorld* World)
{
    if (!World || World->GetNetMode() == NM_Client || !bLoaded)
    {
        return;
    }

    FAetherfrontWorldSaveData SaveData;
    SaveData.AuthoritativeTick = FMath::RoundToInt64(World->GetTimeSeconds() * 20.0);

    for (TActorIterator<AAetherfrontBuilding> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            SaveData.Buildings.Add(It->ToSaveRecord());
        }
    }

    FString Json;
    if (!FJsonObjectConverter::UStructToJsonObjectString(SaveData, Json))
    {
        UE_LOG(LogAetherfront, Error, TEXT("Could not serialize shard state."));
        return;
    }

    const FString SavePath = GetSavePath();
    const FString SaveDirectory = FPaths::GetPath(SavePath);
    IFileManager::Get().MakeDirectory(*SaveDirectory, true);

    const FString TempPath = SavePath + TEXT(".tmp");
    if (!FFileHelper::SaveStringToFile(Json, *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogAetherfront, Error, TEXT("Could not write temporary shard save."));
        return;
    }

    if (!IFileManager::Get().Move(*SavePath, *TempPath, true, true, false, true))
    {
        UE_LOG(LogAetherfront, Error, TEXT("Could not atomically replace shard save."));
        return;
    }

    bDirty = false;
}

FString UAetherfrontPersistenceSubsystem::GetSavePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Aetherfront"), TEXT("world-v1.json"));
}
