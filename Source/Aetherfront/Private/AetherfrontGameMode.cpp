#include "AetherfrontGameMode.h"

#include "Aetherfront.h"
#include "AetherfrontBuilding.h"
#include "AetherfrontCameraPawn.h"
#include "AetherfrontHUD.h"
#include "AetherfrontPersistenceSubsystem.h"
#include "AetherfrontPlayerController.h"
#include "AetherfrontPlayerState.h"
#include "AetherfrontUnit.h"
#include "AetherfrontWorldDirector.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/OnlineReplStructs.h"
#include "TimerManager.h"

AAetherfrontGameMode::AAetherfrontGameMode()
{
    DefaultPawnClass = AAetherfrontCameraPawn::StaticClass();
    PlayerControllerClass = AAetherfrontPlayerController::StaticClass();
    PlayerStateClass = AAetherfrontPlayerState::StaticClass();
    HUDClass = AAetherfrontHUD::StaticClass();
    bUseSeamlessTravel = true;
}

void AAetherfrontGameMode::StartPlay()
{
    Super::StartPlay();

    if (!HasAuthority())
    {
        return;
    }

    bool bHasWorldDirector = false;
    for (TActorIterator<AAetherfrontWorldDirector> It(GetWorld()); It; ++It)
    {
        bHasWorldDirector = true;
        break;
    }

    if (!bHasWorldDirector)
    {
        GetWorld()->SpawnActor<AAetherfrontWorldDirector>(AAetherfrontWorldDirector::StaticClass(), FTransform::Identity);
        UE_LOG(LogAetherfront, Display, TEXT("Created Aetherfront world director."));
    }

    if (UAetherfrontPersistenceSubsystem* Persistence = GetGameInstance()->GetSubsystem<UAetherfrontPersistenceSubsystem>())
    {
        Persistence->LoadWorld(GetWorld());
    }

    GetWorldTimerManager().SetTimer(PersistenceTimer, this, &AAetherfrontGameMode::SaveShard, 15.0f, true, 10.0f);
}

FString AAetherfrontGameMode::InitNewPlayer(
    APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
    if (!Error.IsEmpty())
    {
        return Error;
    }

    FString RequestedId = UGameplayStatics::ParseOption(Options, TEXT("PlayerId"));
    if (RequestedId.IsEmpty() && UniqueId.IsValid())
    {
        RequestedId = UniqueId.ToString();
    }

    FString SanitizedId;
    SanitizedId.Reserve(FMath::Min(64, RequestedId.Len()));
    for (const TCHAR Character : RequestedId.Left(64))
    {
        if (FChar::IsAlnum(Character) || Character == TEXT('-') || Character == TEXT('_') || Character == TEXT('.'))
        {
            SanitizedId.AppendChar(Character);
        }
    }
    if (SanitizedId.IsEmpty())
    {
        SanitizedId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    }

    if (AAetherfrontPlayerState* State = NewPlayerController->GetPlayerState<AAetherfrontPlayerState>())
    {
        State->InitializeIdentityAuthority(SanitizedId);
    }

    return FString();
}

void AAetherfrontGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (AAetherfrontPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<AAetherfrontPlayerState>() : nullptr;
        State && State->GetPersistentPlayerId().IsEmpty())
    {
        State->InitializeIdentityAuthority(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
    }

    UE_LOG(LogAetherfront, Display, TEXT("Commander joined: %s"), *GetNameSafe(NewPlayer));
    SpawnStarterForPlayer(NewPlayer);
}

void AAetherfrontGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SaveShard();
    Super::EndPlay(EndPlayReason);
}

void AAetherfrontGameMode::SpawnStarterForPlayer(APlayerController* NewPlayer)
{
    const AAetherfrontPlayerState* State = NewPlayer
        ? NewPlayer->GetPlayerState<AAetherfrontPlayerState>()
        : nullptr;
    if (!State || State->GetPersistentPlayerId().IsEmpty())
    {
        return;
    }

    const FString PlayerId = State->GetPersistentPlayerId();
    const uint32 Hash = GetTypeHash(PlayerId);
    const float Angle = static_cast<float>(Hash % 10000) / 10000.0f * 2.0f * PI;
    const float SpawnRadius = 9000.0f + static_cast<float>((Hash >> 8) % 4) * 3000.0f;
    FVector BaseLocation(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 125.0f);
    const FLinearColor TeamColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash % 255), 185, 255);

    AAetherfrontBuilding* ExistingCitadel = nullptr;
    for (TActorIterator<AAetherfrontBuilding> It(GetWorld()); It; ++It)
    {
        if (It->IsOwnedBy(PlayerId) && It->GetBuildingKind() == EAetherfrontBuildingKind::Citadel)
        {
            ExistingCitadel = *It;
            BaseLocation = ExistingCitadel->GetActorLocation();
            break;
        }
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (!ExistingCitadel)
    {
        AAetherfrontBuilding* Citadel = GetWorld()->SpawnActor<AAetherfrontBuilding>(
            AAetherfrontBuilding::StaticClass(),
            FTransform(FRotator::ZeroRotator, BaseLocation),
            SpawnParameters);
        if (Citadel)
        {
            Citadel->InitializeAuthority(
                FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower),
                PlayerId,
                EAetherfrontBuildingKind::Citadel,
                TeamColor,
                1.0f);
        }
    }

    bool bHasUnits = false;
    for (TActorIterator<AAetherfrontUnit> It(GetWorld()); It; ++It)
    {
        if (It->IsOwnedBy(PlayerId))
        {
            bHasUnits = true;
            break;
        }
    }

    if (!bHasUnits)
    {
        constexpr int32 StarterUnitCount = 6;
        for (int32 Index = 0; Index < StarterUnitCount; ++Index)
        {
            const float UnitAngle = static_cast<float>(Index) / StarterUnitCount * 2.0f * PI;
            const FVector UnitLocation = BaseLocation + FVector(FMath::Cos(UnitAngle) * 520.0f, FMath::Sin(UnitAngle) * 520.0f, -65.0f);
            AAetherfrontUnit* Unit = GetWorld()->SpawnActor<AAetherfrontUnit>(
                AAetherfrontUnit::StaticClass(),
                FTransform(FRotator(0.0f, UnitAngle * 180.0f / PI, 0.0f), UnitLocation),
                SpawnParameters);
            if (Unit)
            {
                Unit->InitializeAuthority(
                    FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower),
                    PlayerId,
                    Index == StarterUnitCount - 1 ? EAetherfrontUnitKind::Warden : EAetherfrontUnitKind::Fabricator,
                    TeamColor);
            }
        }
    }

    if (APawn* CameraPawn = NewPlayer->GetPawn())
    {
        const FVector CameraLocation = BaseLocation + FVector(0.0f, -1200.0f, -25.0f);
        const FRotator CameraRotation = FRotator::ZeroRotator;
        CameraPawn->SetActorLocationAndRotation(CameraLocation, CameraRotation);
        if (AAetherfrontPlayerController* RTSController = Cast<AAetherfrontPlayerController>(NewPlayer))
        {
            RTSController->ClientInitializeCamera(CameraLocation, CameraRotation);
        }
    }

    if (UAetherfrontPersistenceSubsystem* Persistence = GetGameInstance()->GetSubsystem<UAetherfrontPersistenceSubsystem>())
    {
        Persistence->MarkDirty();
    }
}

void AAetherfrontGameMode::SaveShard()
{
    if (!GetWorld() || !HasAuthority())
    {
        return;
    }

    if (UAetherfrontPersistenceSubsystem* Persistence = GetGameInstance()->GetSubsystem<UAetherfrontPersistenceSubsystem>())
    {
        Persistence->SaveWorld(GetWorld());
    }
}
