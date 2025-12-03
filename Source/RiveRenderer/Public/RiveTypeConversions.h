// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "RiveTypes.h"
THIRD_PARTY_INCLUDES_START
#include "rive/layout.hpp"
THIRD_PARTY_INCLUDES_END

FORCEINLINE rive::Alignment RiveAlignementToAlignment(ERiveAlignment Alignment)
{
    switch (Alignment)
    {
        default:
        case ERiveAlignment::Center:
            return rive::Alignment::center;
        case ERiveAlignment::TopLeft:
            return rive::Alignment::topLeft;
        case ERiveAlignment::TopCenter:
            return rive::Alignment::topCenter;
        case ERiveAlignment::TopRight:
            return rive::Alignment::topRight;
        case ERiveAlignment::CenterLeft:
            return rive::Alignment::centerLeft;
        case ERiveAlignment::CenterRight:
            return rive::Alignment::centerRight;
        case ERiveAlignment::BottomLeft:
            return rive::Alignment::bottomLeft;
        case ERiveAlignment::BottomCenter:
            return rive::Alignment::bottomCenter;
        case ERiveAlignment::BottomRight:
            return rive::Alignment::bottomRight;
    }
}

FORCEINLINE rive::Fit RiveFitTypeToFit(ERiveFitType Fit)
{
    switch (Fit)
    {
        case ERiveFitType::Fill:
            return rive::Fit::fill;
        case ERiveFitType::Contain:
            return rive::Fit::contain;
        case ERiveFitType::Cover:
            return rive::Fit::cover;
        case ERiveFitType::FitWidth:
            return rive::Fit::fitWidth;
        case ERiveFitType::FitHeight:
            return rive::Fit::fitHeight;
        case ERiveFitType::ScaleDown:
            return rive::Fit::scaleDown;
        case ERiveFitType::Layout:
            return rive::Fit::layout;
        default:
        case ERiveFitType::None:
            return rive::Fit::none;
    }
}