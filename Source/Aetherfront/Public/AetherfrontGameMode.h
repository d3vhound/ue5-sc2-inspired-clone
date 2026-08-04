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
    virtual void PostLogin(APlayerController* NewPlayer) override;
};

