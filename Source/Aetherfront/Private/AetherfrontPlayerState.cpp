#include "AetherfrontPlayerState.h"

#include "Net/UnrealNetwork.h"

AAetherfrontPlayerState::AAetherfrontPlayerState()
{
    SetNetUpdateFrequency(4.0f);
}

void AAetherfrontPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAetherfrontPlayerState, PersistentPlayerId);
    DOREPLIFETIME(AAetherfrontPlayerState, Alloy);
    DOREPLIFETIME(AAetherfrontPlayerState, Flux);
}

void AAetherfrontPlayerState::InitializeIdentityAuthority(const FString& RequestedId)
{
    check(HasAuthority());
    PersistentPlayerId = RequestedId.Left(64);
}

bool AAetherfrontPlayerState::TrySpendAlloyAuthority(const int32 Amount)
{
    check(HasAuthority());

    if (Amount < 0 || Alloy < Amount)
    {
        return false;
    }

    Alloy -= Amount;
    ForceNetUpdate();
    return true;
}

void AAetherfrontPlayerState::AddAlloyAuthority(const int32 Amount)
{
    check(HasAuthority());
    Alloy = FMath::Max(0, Alloy + Amount);
    ForceNetUpdate();
}
