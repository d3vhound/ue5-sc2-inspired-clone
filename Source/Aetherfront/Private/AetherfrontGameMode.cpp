#include "AetherfrontGameMode.h"

#include "Aetherfront.h"
#include "AetherfrontCameraPawn.h"
#include "AetherfrontPlayerController.h"
#include "AetherfrontWorldDirector.h"
#include "EngineUtils.h"

AAetherfrontGameMode::AAetherfrontGameMode()
{
    DefaultPawnClass = AAetherfrontCameraPawn::StaticClass();
    PlayerControllerClass = AAetherfrontPlayerController::StaticClass();
    bUseSeamlessTravel = true;
}

void AAetherfrontGameMode::StartPlay()
{
    Super::StartPlay();

    if (!HasAuthority())
    {
        return;
    }

    for (TActorIterator<AAetherfrontWorldDirector> It(GetWorld()); It; ++It)
    {
        return;
    }

    GetWorld()->SpawnActor<AAetherfrontWorldDirector>(AAetherfrontWorldDirector::StaticClass(), FTransform::Identity);
    UE_LOG(LogAetherfront, Display, TEXT("Created Aetherfront world director."));
}

void AAetherfrontGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    UE_LOG(LogAetherfront, Display, TEXT("Commander joined: %s"), *GetNameSafe(NewPlayer));
}

