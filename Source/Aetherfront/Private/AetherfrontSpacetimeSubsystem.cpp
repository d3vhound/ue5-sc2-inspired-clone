#include "AetherfrontSpacetimeSubsystem.h"

#include "Aetherfront.h"
#include "Connection/Credentials.h"

void UAetherfrontSpacetimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (bAutoConnect)
    {
        ConnectBackend();
    }
}

void UAetherfrontSpacetimeSubsystem::Deinitialize()
{
    DisconnectBackend();
    Super::Deinitialize();
}

void UAetherfrontSpacetimeSubsystem::ConnectBackend()
{
    if (Connection != nullptr)
    {
        return;
    }

    SetBackendStatus(EAetherfrontBackendStatus::Connecting);

    FOnConnectDelegate ConnectDelegate;
    BIND_DELEGATE_SAFE(
        ConnectDelegate,
        this,
        UAetherfrontSpacetimeSubsystem,
        HandleConnect);
    FOnDisconnectDelegate DisconnectDelegate;
    BIND_DELEGATE_SAFE(
        DisconnectDelegate,
        this,
        UAetherfrontSpacetimeSubsystem,
        HandleDisconnect);
    FOnConnectErrorDelegate ConnectErrorDelegate;
    BIND_DELEGATE_SAFE(
        ConnectErrorDelegate,
        this,
        UAetherfrontSpacetimeSubsystem,
        HandleConnectError);

    UCredentials::Init(TokenFilePath);
    const FString SavedToken = UCredentials::LoadToken();

    UDbConnectionBuilder* Builder = UDbConnection::Builder()
        ->WithUri(ServerUri)
        ->WithDatabaseName(DatabaseName)
        ->OnConnect(ConnectDelegate)
        ->OnDisconnect(DisconnectDelegate)
        ->OnConnectError(ConnectErrorDelegate);
    if (!SavedToken.IsEmpty())
    {
        Builder->WithToken(SavedToken);
    }

    Connection = Builder->Build();
    if (Connection == nullptr)
    {
        SetBackendStatus(
            EAetherfrontBackendStatus::Error,
            TEXT("SpacetimeDB connection builder returned no connection"));
        return;
    }

    // SpacetimeDB dispatches network callbacks from FrameTick. Auto-ticking keeps
    // that pump alive without making the GameInstance subsystem itself tickable.
    Connection->SetAutoTicking(true);
}

void UAetherfrontSpacetimeSubsystem::DisconnectBackend()
{
    if (Connection != nullptr)
    {
        Connection->SetAutoTicking(false);
        Connection->Disconnect();
        Connection = nullptr;
    }
    SetBackendStatus(EAetherfrontBackendStatus::Disconnected);
}

bool UAetherfrontSpacetimeSubsystem::IsConnected() const
{
    return Connection != nullptr && Connection->IsActive();
}

void UAetherfrontSpacetimeSubsystem::SetBackendStatus(
    EAetherfrontBackendStatus NewStatus,
    const FString& Error)
{
    LastConnectionError = Error;
    if (BackendStatus == NewStatus)
    {
        return;
    }
    BackendStatus = NewStatus;
    OnBackendStatusChanged.Broadcast(NewStatus);
}

void UAetherfrontSpacetimeSubsystem::HandleConnect(
    UDbConnection* InConnection,
    FSpacetimeDBIdentity Identity,
    const FString& Token)
{
    Connection = InConnection;
    LocalIdentity = Identity;
    UCredentials::SaveToken(Token);
    SetBackendStatus(EAetherfrontBackendStatus::Synchronizing);

    FOnSubscriptionApplied AppliedDelegate;
    BIND_DELEGATE_SAFE(
        AppliedDelegate,
        this,
        UAetherfrontSpacetimeSubsystem,
        HandleSubscriptionApplied);
    Connection->SubscriptionBuilder()
        ->OnApplied(AppliedDelegate)
        ->SubscribeToAllTables();
}

void UAetherfrontSpacetimeSubsystem::HandleConnectError(const FString& Error)
{
    UE_LOG(LogAetherfront, Error, TEXT("SpacetimeDB connection failed: %s"), *Error);
    if (Connection != nullptr)
    {
        Connection->SetAutoTicking(false);
        Connection = nullptr;
    }
    SetBackendStatus(EAetherfrontBackendStatus::Error, Error);
}

void UAetherfrontSpacetimeSubsystem::HandleDisconnect(
    UDbConnection* InConnection,
    const FString& Error)
{
    if (InConnection != nullptr)
    {
        InConnection->SetAutoTicking(false);
    }
    Connection = nullptr;
    if (Error.IsEmpty())
    {
        UE_LOG(LogAetherfront, Log, TEXT("SpacetimeDB disconnected."));
        SetBackendStatus(EAetherfrontBackendStatus::Disconnected);
    }
    else
    {
        UE_LOG(LogAetherfront, Warning, TEXT("SpacetimeDB disconnected: %s"), *Error);
        SetBackendStatus(EAetherfrontBackendStatus::Error, Error);
    }
}

void UAetherfrontSpacetimeSubsystem::HandleSubscriptionApplied(
    FSubscriptionEventContext& Context)
{
    SetBackendStatus(EAetherfrontBackendStatus::Ready);
    UE_LOG(
        LogAetherfront,
        Log,
        TEXT("SpacetimeDB subscription synchronized; persistent shard is ready."));
}
