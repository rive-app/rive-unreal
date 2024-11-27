// Copyright Rive, Inc. All rights reserved.

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
    Fill = 0,
    Contain = 1,
    Cover = 2,
    FitWidth = 3,
    FitHeight = 4,
    None = 5,
    ScaleDown = 6,
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

UENUM(BlueprintType)
enum class ERiveInitState : uint8
{
    Uninitialized = 0,
    Deinitializing = 1,
    Initializing = 2,
    Initialized = 3,
};
