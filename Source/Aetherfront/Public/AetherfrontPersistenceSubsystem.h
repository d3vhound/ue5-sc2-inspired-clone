#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AetherfrontPersistenceSubsystem.generated.h"

UCLASS()
class AETHERFRONT_API UAetherfrontPersistenceSubsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    void LoadWorld(UWorld* World);
    void SaveWorld(UWorld* World);
    void MarkDirty() { bDirty = true; }

    bool HasLoaded() const { return bLoaded; }
    bool IsDirty() const { return bDirty; }

private:
    FString GetSavePath() const;

    bool bLoaded = false;
    bool bDirty = false;
};
