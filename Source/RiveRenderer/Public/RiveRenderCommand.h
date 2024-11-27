// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveTypes.h"
THIRD_PARTY_INCLUDES_START
#include "rive/math/mat2d.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveRenderCommand.generated.h"

namespace rive
{
class Artboard;
}

UENUM(BlueprintType)
enum class ERiveRenderCommandType : uint8
{
    Save = 0,
    Restore,
    Transform,
    DrawArtboard,
    DrawPath,
    ClipPath,
    AlignArtboard,
    Translate
};

USTRUCT(BlueprintType)
struct FRiveRenderCommand
{
public:
    GENERATED_BODY()

    FRiveRenderCommand() :
        Type(),
        FitType(),
        ScaleFactor(1.0f),
        X(0),
        Y(0),
        X2(0),
        Y2(0),
        TX(0),
        TY(0)
    {}

    explicit FRiveRenderCommand(ERiveRenderCommandType InType) :
        Type(InType),
        FitType(),
        ScaleFactor(1.0f),
        X(0),
        Y(0),
        X2(0),
        Y2(0),
        TX(0),
        TY(0)
    {}

    explicit FRiveRenderCommand(const rive::Mat2D& Matrix) :
        Type(ERiveRenderCommandType::Transform),
        FitType(),
        ScaleFactor(1.0f),
        X(Matrix.xx()),
        Y(Matrix.xy()),
        X2(Matrix.yx()),
        Y2(Matrix.yy()),
        TX(Matrix.tx()),
        TY(Matrix.ty())
    {}

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    ERiveRenderCommandType Type;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    ERiveFitType FitType;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float ScaleFactor;

    // UPROPERTY(BlueprintReadWrite)
    rive::Artboard* NativeArtboard = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float X;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float Y;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float X2;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float Y2;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float TX;

    UPROPERTY(BlueprintReadWrite, Category = Rive)
    float TY;

    rive::Mat2D GetSaved2DTransform() const;

    FMatrix GetSavedTransform() const
    {
        const rive::Mat2D Mat2d = GetSaved2DTransform();
        return FMatrix(FVector{Mat2d.xx(), Mat2d.xy(), 0},
                       FVector{Mat2d.yx(), Mat2d.yy(), 0},
                       FVector{0, 0, 1},
                       FVector{Mat2d.tx(), Mat2d.ty(), 0});
    }
};
