#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AetherfrontHUD.generated.h"

UCLASS()
class AETHERFRONT_API AAetherfrontHUD final : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    void BeginSelection(const FVector2D& ScreenPoint);
    void UpdateSelection(const FVector2D& ScreenPoint);
    void EndSelection();
    void SetSelectionCount(int32 InSelectionCount);
    void SetBuildMode(bool bInBuildMode);

private:
    bool bDrawingSelection = false;
    bool bBuildMode = false;
    int32 SelectionCount = 0;
    FVector2D SelectionStart = FVector2D::ZeroVector;
    FVector2D SelectionEnd = FVector2D::ZeroVector;
};
