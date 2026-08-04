#include "AetherfrontHUD.h"

#include "AetherfrontPlayerState.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AAetherfrontHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas)
    {
        return;
    }

    const FLinearColor Primary(0.62f, 0.95f, 0.88f, 1.0f);
    const FLinearColor Muted(0.65f, 0.70f, 0.74f, 1.0f);
    DrawText(TEXT("AETHERFRONT  //  FRONTIER SHARD"), Primary, 28.0f, 24.0f, GEngine->GetSmallFont(), 1.15f, false);

    const AAetherfrontPlayerState* State = PlayerOwner
        ? PlayerOwner->GetPlayerState<AAetherfrontPlayerState>()
        : nullptr;
    if (State)
    {
        const FString Economy = FString::Printf(TEXT("ALLOY  %d     FLUX  %d"), State->GetAlloy(), State->GetFlux());
        DrawText(Economy, FLinearColor::White, Canvas->SizeX - 245.0f, 24.0f, GEngine->GetSmallFont(), 1.0f, false);
    }

    const FString Selection = FString::Printf(TEXT("SELECTED  %d"), SelectionCount);
    DrawText(Selection, Muted, 28.0f, Canvas->SizeY - 72.0f, GEngine->GetSmallFont(), 1.0f, false);

    const FString Controls = bBuildMode
        ? TEXT("PLACE RELAY  //  LEFT CLICK TO CONFIRM  //  B TO CANCEL")
        : TEXT("WASD PAN   WHEEL ZOOM   Q/E ROTATE   DRAG SELECT   RIGHT CLICK MOVE   B BUILD");
    DrawText(
        Controls,
        bBuildMode ? FLinearColor(1.0f, 0.72f, 0.25f) : Muted,
        28.0f,
        Canvas->SizeY - 42.0f,
        GEngine->GetSmallFont(),
        0.95f,
        false);

    if (bDrawingSelection)
    {
        const FVector2D Min(FMath::Min(SelectionStart.X, SelectionEnd.X), FMath::Min(SelectionStart.Y, SelectionEnd.Y));
        const FVector2D Max(FMath::Max(SelectionStart.X, SelectionEnd.X), FMath::Max(SelectionStart.Y, SelectionEnd.Y));
        const FVector2D Size = Max - Min;

        DrawRect(FLinearColor(0.08f, 0.82f, 0.72f, 0.10f), Min.X, Min.Y, Size.X, Size.Y);
        DrawLine(Min.X, Min.Y, Max.X, Min.Y, Primary, 1.0f);
        DrawLine(Max.X, Min.Y, Max.X, Max.Y, Primary, 1.0f);
        DrawLine(Max.X, Max.Y, Min.X, Max.Y, Primary, 1.0f);
        DrawLine(Min.X, Max.Y, Min.X, Min.Y, Primary, 1.0f);
    }
}

void AAetherfrontHUD::BeginSelection(const FVector2D& ScreenPoint)
{
    bDrawingSelection = true;
    SelectionStart = ScreenPoint;
    SelectionEnd = ScreenPoint;
}

void AAetherfrontHUD::UpdateSelection(const FVector2D& ScreenPoint)
{
    SelectionEnd = ScreenPoint;
}

void AAetherfrontHUD::EndSelection()
{
    bDrawingSelection = false;
}

void AAetherfrontHUD::SetSelectionCount(const int32 InSelectionCount)
{
    SelectionCount = FMath::Max(0, InSelectionCount);
}

void AAetherfrontHUD::SetBuildMode(const bool bInBuildMode)
{
    bBuildMode = bInBuildMode;
}
