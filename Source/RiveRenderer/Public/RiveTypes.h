// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveTypes.generated.h"

USTRUCT(Blueprintable)
struct RIVERENDERER_API FRiveStateMachineEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    FIntPoint MousePosition = FIntPoint(0, 0);
};

UENUM(BlueprintType)
enum class ERiveFitType : uint8
{
    // Fill the bounds by scaling the artboard. Does not maintain aspect.
    Fill = 0,
    // The same as FitWidth or FitHeight except chooses the largest side to fit,
    // so that the entire artboard is always within the given bounds
    Contain = 1,
    // The same as Contain except it fits the smallest side, so that some of the
    // artboard is always outside of the bounds but the entire bounds is always
    // covered.
    Cover = 2,
    // Scale the artboard width so that the width completely fits within the
    // given bounds while maintaining the aspect ratio.
    FitWidth = 3,
    // Scale the artboard width so that the height completely fits within the
    // given bounds while maintaining the aspect ratio.
    FitHeight = 4,
    None = 5,
    // Scale down the artboard size so that it fits within the bounds
    ScaleDown = 6,
    // Instead of scaling the artboard to fit the bounds, fill the space using
    // layouts. Only works if layouts are used in the riv file.
    Layout = 7
};

UENUM(BlueprintType, meta = (ScriptName = "ERiveAlignment"))
enum class ERiveAlignment : uint8
{
    TopLeft = 0,
    TopCenter = 1,
    TopRight = 2,
    CenterLeft = 3,
    Center = 4,
    CenterRight = 5,
    BottomLeft = 6,
    BottomCenter = 7,
    BottomRight = 8,
};

USTRUCT(BlueprintType)
struct RIVERENDERER_API FRiveAlignment
{
    GENERATED_BODY()

public:
    inline static FVector2f TopLeft = FVector2f(-1.f, -1.f);
    inline static FVector2f TopCenter = FVector2f(0.f, -1.f);
    inline static FVector2f TopRight = FVector2f(1.f, -1.f);
    inline static FVector2f CenterLeft = FVector2f(-1.f, 0.f);
    inline static FVector2f Center = FVector2f(0.f, 0.f);
    inline static FVector2f CenterRight = FVector2f(1.f, 0.f);
    inline static FVector2f BottomLeft = FVector2f(-1.f, 1.f);
    inline static FVector2f BottomCenter = FVector2f(0.f, 1.f);
    inline static FVector2f BottomRight = FVector2f(1.f, 1.f);

    static FVector2f GetAlignment(ERiveAlignment InRiveAlignment)
    {
        switch (InRiveAlignment)
        {
            case ::ERiveAlignment::TopLeft:
                return TopLeft;
            case ::ERiveAlignment::TopCenter:
                return TopCenter;
            case ::ERiveAlignment::TopRight:
                return TopRight;
            case ::ERiveAlignment::CenterLeft:
                return CenterLeft;
            case ::ERiveAlignment::Center:
                return Center;
            case ::ERiveAlignment::CenterRight:
                return CenterRight;
            case ::ERiveAlignment::BottomLeft:
                return BottomLeft;
            case ::ERiveAlignment::BottomCenter:
                return BottomCenter;
            case ::ERiveAlignment::BottomRight:
                return BottomRight;
        }

        return Center;
    }
};
