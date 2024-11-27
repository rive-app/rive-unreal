// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderCommand.h"

THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END

using namespace rive;
Mat2D FRiveRenderCommand::GetSaved2DTransform() const
{
    switch (Type)
    {
        case ERiveRenderCommandType::Transform:
            return Mat2D(X, Y, X2, Y2, TX, TY);
        case ERiveRenderCommandType::AlignArtboard:
            return computeAlignment(static_cast<Fit>(FitType),
                                    Alignment(X, Y),
                                    AABB(TX, TY, X2, Y2),
                                    NativeArtboard->bounds(),
                                    ScaleFactor);
        case ERiveRenderCommandType::Translate:
            return Mat2D(1.f, 0.f, 0.f, 1.f, TX, TY);
        default:
            return Mat2D();
    }
}
