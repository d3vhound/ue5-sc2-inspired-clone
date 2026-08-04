#include "AetherfrontPlayerController.h"

#include "Aetherfront.h"
#include "AetherfrontBuilding.h"
#include "AetherfrontHUD.h"
#include "AetherfrontPersistenceSubsystem.h"
#include "AetherfrontPlayerState.h"
#include "AetherfrontResourceNode.h"
#include "AetherfrontUnit.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

AAetherfrontPlayerController::AAetherfrontPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AAetherfrontPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        UpdateHUD();
    }
}

void AAetherfrontPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);
    InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &AAetherfrontPlayerController::HandleSelectPressed);
    InputComponent->BindAction(TEXT("Select"), IE_Released, this, &AAetherfrontPlayerController::HandleSelectReleased);
    InputComponent->BindAction(TEXT("Command"), IE_Pressed, this, &AAetherfrontPlayerController::HandleCommand);
    InputComponent->BindAction(TEXT("Build"), IE_Pressed, this, &AAetherfrontPlayerController::HandleBuildToggle);
}

void AAetherfrontPlayerController::PlayerTick(const float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!IsLocalController())
    {
        return;
    }

    if (bHasPendingCameraTransform)
    {
        if (APawn* ControlledPawn = GetPawn())
        {
            ControlledPawn->SetActorLocationAndRotation(PendingCameraLocation, PendingCameraRotation);
            bHasPendingCameraTransform = false;
        }
    }

    if (!bSelecting)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionCurrent = FVector2D(MouseX, MouseY);
        if (AAetherfrontHUD* RTSHUD = Cast<AAetherfrontHUD>(GetHUD()))
        {
            RTSHUD->UpdateSelection(SelectionCurrent);
        }
    }
}

void AAetherfrontPlayerController::ClientInitializeCamera_Implementation(
    const FVector_NetQuantize10 Location,
    const FRotator Rotation)
{
    PendingCameraLocation = Location;
    PendingCameraRotation = Rotation;
    bHasPendingCameraTransform = true;

    if (APawn* ControlledPawn = GetPawn())
    {
        ControlledPawn->SetActorLocationAndRotation(PendingCameraLocation, PendingCameraRotation);
        bHasPendingCameraTransform = false;
    }
}

void AAetherfrontPlayerController::HandleSelectPressed()
{
    if (bBuildMode)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    bSelecting = true;
    SelectionStart = FVector2D(MouseX, MouseY);
    SelectionCurrent = SelectionStart;
    if (AAetherfrontHUD* RTSHUD = Cast<AAetherfrontHUD>(GetHUD()))
    {
        RTSHUD->BeginSelection(SelectionStart);
    }
}

void AAetherfrontPlayerController::HandleSelectReleased()
{
    if (bBuildMode)
    {
        FHitResult Hit;
        if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
        {
            ServerPlaceBuilding(EAetherfrontBuildingKind::Relay, Hit.ImpactPoint);
        }
        bBuildMode = false;
        UpdateHUD();
        return;
    }

    if (!bSelecting)
    {
        return;
    }

    bSelecting = false;
    if (AAetherfrontHUD* RTSHUD = Cast<AAetherfrontHUD>(GetHUD()))
    {
        RTSHUD->EndSelection();
    }

    const bool bAddToSelection = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(SelectionStart, SelectionCurrent) <= 7.0f)
    {
        FHitResult Hit;
        if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
        {
            SelectSingleActor(Hit.GetActor(), bAddToSelection);
        }
        else if (!bAddToSelection)
        {
            ClearSelection();
        }
    }
    else
    {
        SelectUnitsInRectangle(SelectionStart, SelectionCurrent, bAddToSelection);
    }

    UpdateHUD();
}

void AAetherfrontPlayerController::HandleCommand()
{
    if (SelectedUnits.IsEmpty())
    {
        return;
    }

    FHitResult Hit;
    if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
    {
        return;
    }

    TArray<AAetherfrontUnit*> Units;
    Units.Reserve(SelectedUnits.Num());
    for (AAetherfrontUnit* Unit : SelectedUnits)
    {
        if (IsValid(Unit))
        {
            Units.Add(Unit);
        }
    }

    const bool bQueue = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    ServerIssueMove(Units, Hit.ImpactPoint, bQueue, NextCommandSequence++);
}

void AAetherfrontPlayerController::HandleBuildToggle()
{
    bBuildMode = !bBuildMode;
    if (bSelecting)
    {
        bSelecting = false;
        if (AAetherfrontHUD* RTSHUD = Cast<AAetherfrontHUD>(GetHUD()))
        {
            RTSHUD->EndSelection();
        }
    }
    UpdateHUD();
}

void AAetherfrontPlayerController::ClearSelection()
{
    for (AAetherfrontUnit* Unit : SelectedUnits)
    {
        if (IsValid(Unit))
        {
            Unit->SetLocallySelected(false);
        }
    }
    SelectedUnits.Reset();

    if (IsValid(SelectedBuilding))
    {
        SelectedBuilding->SetLocallySelected(false);
    }
    SelectedBuilding = nullptr;
}

void AAetherfrontPlayerController::SelectSingleActor(AActor* Actor, const bool bAddToSelection)
{
    if (!bAddToSelection)
    {
        ClearSelection();
    }

    const FString PlayerId = GetPersistentPlayerId();
    if (AAetherfrontUnit* Unit = Cast<AAetherfrontUnit>(Actor); Unit && Unit->IsOwnedBy(PlayerId))
    {
        if (!SelectedUnits.Contains(Unit))
        {
            SelectedUnits.Add(Unit);
            Unit->SetLocallySelected(true);
        }
        return;
    }

    if (AAetherfrontBuilding* Building = Cast<AAetherfrontBuilding>(Actor); Building && Building->IsOwnedBy(PlayerId))
    {
        if (IsValid(SelectedBuilding) && SelectedBuilding != Building)
        {
            SelectedBuilding->SetLocallySelected(false);
        }
        SelectedBuilding = Building;
        Building->SetLocallySelected(true);
    }
}

void AAetherfrontPlayerController::SelectUnitsInRectangle(
    const FVector2D& Start,
    const FVector2D& End,
    const bool bAddToSelection)
{
    if (!bAddToSelection)
    {
        ClearSelection();
    }

    FBox2D SelectionBox(ForceInit);
    SelectionBox += Start;
    SelectionBox += End;
    const FString PlayerId = GetPersistentPlayerId();
    for (TActorIterator<AAetherfrontUnit> It(GetWorld()); It; ++It)
    {
        if (!It->IsOwnedBy(PlayerId))
        {
            continue;
        }

        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(It->GetActorLocation(), ScreenPosition, true)
            && SelectionBox.IsInside(ScreenPosition)
            && !SelectedUnits.Contains(*It))
        {
            SelectedUnits.Add(*It);
            It->SetLocallySelected(true);
        }
    }
}

void AAetherfrontPlayerController::UpdateHUD()
{
    if (AAetherfrontHUD* RTSHUD = Cast<AAetherfrontHUD>(GetHUD()))
    {
        RTSHUD->SetSelectionCount(SelectedUnits.Num() + (IsValid(SelectedBuilding) ? 1 : 0));
        RTSHUD->SetBuildMode(bBuildMode);
    }
}

FString AAetherfrontPlayerController::GetPersistentPlayerId() const
{
    const AAetherfrontPlayerState* State = GetPlayerState<AAetherfrontPlayerState>();
    return State ? State->GetPersistentPlayerId() : FString();
}

bool AAetherfrontPlayerController::IsBuildingLocationValid(
    const EAetherfrontBuildingKind Kind,
    const FVector& Location) const
{
    if (Location.ContainsNaN() || FMath::Abs(Location.X) > 900000.0f || FMath::Abs(Location.Y) > 900000.0f)
    {
        return false;
    }

    const float Footprint = AAetherfrontBuilding::GetFootprintRadius(Kind);
    for (TActorIterator<AAetherfrontBuilding> It(GetWorld()); It; ++It)
    {
        const float RequiredSpacing = Footprint + AAetherfrontBuilding::GetFootprintRadius(It->GetBuildingKind()) + 70.0f;
        if (FVector::DistSquared2D(Location, It->GetActorLocation()) < FMath::Square(RequiredSpacing))
        {
            return false;
        }
    }

    for (TActorIterator<AAetherfrontResourceNode> It(GetWorld()); It; ++It)
    {
        if (FVector::DistSquared2D(Location, It->GetActorLocation()) < FMath::Square(Footprint + 180.0f))
        {
            return false;
        }
    }

    for (TActorIterator<AAetherfrontUnit> It(GetWorld()); It; ++It)
    {
        if (FVector::DistSquared2D(Location, It->GetActorLocation()) < FMath::Square(Footprint + 90.0f))
        {
            return false;
        }
    }

    return true;
}

void AAetherfrontPlayerController::ServerIssueMove_Implementation(
    const TArray<AAetherfrontUnit*>& Units,
    const FVector_NetQuantize10 Target,
    const bool bQueue,
    const uint32 CommandSequence)
{
    const FString PlayerId = GetPersistentPlayerId();
    const FVector RequestedTarget(Target);
    constexpr float WorldHalfExtent = 900000.0f;
    if (PlayerId.IsEmpty()
        || RequestedTarget.ContainsNaN()
        || FMath::Abs(RequestedTarget.X) > WorldHalfExtent
        || FMath::Abs(RequestedTarget.Y) > WorldHalfExtent
        || Units.IsEmpty()
        || Units.Num() > 200)
    {
        return;
    }

    const double ServerTime = GetWorld()->GetTimeSeconds();
    constexpr double MinimumMoveInterval = 0.025;
    if (LastMoveCommandServerTime >= 0.0 && ServerTime - LastMoveCommandServerTime < MinimumMoveInterval)
    {
        return;
    }
    LastMoveCommandServerTime = ServerTime;

    const int32 ColumnCount = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Units.Num())));
    constexpr float Spacing = 145.0f;
    int32 ValidIndex = 0;
    for (AAetherfrontUnit* Unit : Units)
    {
        if (!IsValid(Unit) || !Unit->IsOwnedBy(PlayerId))
        {
            continue;
        }

        const int32 Row = ValidIndex / ColumnCount;
        const int32 Column = ValidIndex % ColumnCount;
        const float Center = static_cast<float>(ColumnCount - 1) * 0.5f;
        const FVector FormationOffset((static_cast<float>(Column) - Center) * Spacing, (static_cast<float>(Row) - Center) * Spacing, 0.0f);
        Unit->SetMoveTargetAuthority(RequestedTarget + FormationOffset, CommandSequence, bQueue);
        ++ValidIndex;
    }

    UE_LOG(LogAetherfront, Verbose, TEXT("Accepted command %u for %d units (queued=%d)."), CommandSequence, ValidIndex, bQueue);
}

void AAetherfrontPlayerController::ServerPlaceBuilding_Implementation(
    const EAetherfrontBuildingKind Kind,
    const FVector_NetQuantize10 Location)
{
    if (Kind != EAetherfrontBuildingKind::Citadel
        && Kind != EAetherfrontBuildingKind::Relay
        && Kind != EAetherfrontBuildingKind::Extractor)
    {
        return;
    }

    const double ServerTime = GetWorld()->GetTimeSeconds();
    constexpr double MinimumBuildInterval = 0.20;
    if (LastBuildCommandServerTime >= 0.0 && ServerTime - LastBuildCommandServerTime < MinimumBuildInterval)
    {
        return;
    }
    LastBuildCommandServerTime = ServerTime;

    AAetherfrontPlayerState* State = GetPlayerState<AAetherfrontPlayerState>();
    const FString PlayerId = State ? State->GetPersistentPlayerId() : FString();
    if (!State || PlayerId.IsEmpty() || !IsBuildingLocationValid(Kind, Location))
    {
        return;
    }

    const int32 Cost = AAetherfrontBuilding::GetAlloyCost(Kind);
    if (!State->TrySpendAlloyAuthority(Cost))
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    FVector SpawnLocation = Location;
    SpawnLocation.Z = Kind == EAetherfrontBuildingKind::Citadel ? 125.0f : 95.0f;
    AAetherfrontBuilding* Building = GetWorld()->SpawnActor<AAetherfrontBuilding>(
        AAetherfrontBuilding::StaticClass(),
        FTransform(FRotator::ZeroRotator, SpawnLocation),
        SpawnParameters);
    if (!Building)
    {
        State->AddAlloyAuthority(Cost);
        return;
    }

    const uint32 Hash = GetTypeHash(PlayerId);
    const FLinearColor TeamColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash % 255), 185, 255);
    Building->InitializeAuthority(
        FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower),
        PlayerId,
        Kind,
        TeamColor,
        0.0f);

    if (UAetherfrontPersistenceSubsystem* Persistence = GetGameInstance()->GetSubsystem<UAetherfrontPersistenceSubsystem>())
    {
        Persistence->MarkDirty();
    }
}
