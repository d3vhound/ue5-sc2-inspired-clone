#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AetherfrontGameMode.generated.h"

UCLASS()
class AETHERFRONT_API AAetherfrontGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAetherfrontGameMode();

    virtual void StartPlay() override;
    virtual FString InitNewPlayer(
        APlayerController* NewPlayerController,
        const FUniqueNetIdRepl& UniqueId,
        const FString& Options,
        const FString& Portal = TEXT("")) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SpawnStarterForPlayer(APlayerController* NewPlayer);
    void SaveShard();

    FTimerHandle PersistenceTimer;
};
