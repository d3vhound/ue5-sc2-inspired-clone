#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModuleBindings/SpacetimeDBClient.g.h"
#include "AetherfrontSpacetimeSubsystem.generated.h"

class UDbConnection;

UENUM(BlueprintType)
enum class EAetherfrontBackendStatus : uint8
{
    Disconnected,
    Connecting,
    Synchronizing,
    Ready,
    Error
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FAetherfrontBackendStatusChanged,
    EAetherfrontBackendStatus,
    NewStatus);

UCLASS(Config=Game, DefaultConfig)
class AETHERFRONT_API UAetherfrontSpacetimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Aetherfront|Backend")
    void ConnectBackend();

    UFUNCTION(BlueprintCallable, Category="Aetherfront|Backend")
    void DisconnectBackend();

    UFUNCTION(BlueprintPure, Category="Aetherfront|Backend")
    bool IsConnected() const;

    UFUNCTION(BlueprintPure, Category="Aetherfront|Backend")
    bool IsReady() const { return BackendStatus == EAetherfrontBackendStatus::Ready; }

    UPROPERTY(BlueprintAssignable, Category="Aetherfront|Backend")
    FAetherfrontBackendStatusChanged OnBackendStatusChanged;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Aetherfront|Backend")
    EAetherfrontBackendStatus BackendStatus = EAetherfrontBackendStatus::Disconnected;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Aetherfront|Backend")
    FString LastConnectionError;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Aetherfront|Backend")
    FSpacetimeDBIdentity LocalIdentity;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Aetherfront|Backend")
    TObjectPtr<UDbConnection> Connection;

private:
    UPROPERTY(Config, EditAnywhere, Category="Aetherfront|Backend", meta=(AllowPrivateAccess="true"))
    FString ServerUri = TEXT("127.0.0.1:3000");

    UPROPERTY(Config, EditAnywhere, Category="Aetherfront|Backend", meta=(AllowPrivateAccess="true"))
    FString DatabaseName = TEXT("aetherfront-dev");

    UPROPERTY(Config, EditAnywhere, Category="Aetherfront|Backend", meta=(AllowPrivateAccess="true"))
    FString TokenFilePath = TEXT(".spacetime_aetherfront");

    UPROPERTY(Config, EditAnywhere, Category="Aetherfront|Backend", meta=(AllowPrivateAccess="true"))
    bool bAutoConnect = true;

    void SetBackendStatus(EAetherfrontBackendStatus NewStatus, const FString& Error = FString());

    UFUNCTION()
    void HandleConnect(
        UDbConnection* InConnection,
        FSpacetimeDBIdentity Identity,
        const FString& Token);

    UFUNCTION()
    void HandleConnectError(const FString& Error);

    UFUNCTION()
    void HandleDisconnect(UDbConnection* InConnection, const FString& Error);

    UFUNCTION()
    void HandleSubscriptionApplied(FSubscriptionEventContext& Context);
};
