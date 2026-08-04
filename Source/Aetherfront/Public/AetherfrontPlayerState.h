#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AetherfrontPlayerState.generated.h"

UCLASS()
class AETHERFRONT_API AAetherfrontPlayerState final : public APlayerState
{
    GENERATED_BODY()

public:
    AAetherfrontPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeIdentityAuthority(const FString& RequestedId);
    bool TrySpendAlloyAuthority(int32 Amount);
    void AddAlloyAuthority(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Commander")
    const FString& GetPersistentPlayerId() const { return PersistentPlayerId; }

    UFUNCTION(BlueprintPure, Category = "Economy")
    int32 GetAlloy() const { return Alloy; }

    UFUNCTION(BlueprintPure, Category = "Economy")
    int32 GetFlux() const { return Flux; }

private:
    UPROPERTY(Replicated)
    FString PersistentPlayerId;

    UPROPERTY(Replicated)
    int32 Alloy = 1200;

    UPROPERTY(Replicated)
    int32 Flux = 200;
};
