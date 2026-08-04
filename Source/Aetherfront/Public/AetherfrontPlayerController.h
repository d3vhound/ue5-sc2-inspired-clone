#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AetherfrontPlayerController.generated.h"

UCLASS()
class AETHERFRONT_API AAetherfrontPlayerController final : public APlayerController
{
    GENERATED_BODY()

public:
    AAetherfrontPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void HandleSelect();
    void HandleCommand();
};

