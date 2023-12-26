// Copyright Rive, Inc. All rights reserved.

#pragma once

namespace UE::Rive::Core
{
    enum class EPaintType : uint8
    {
        None,

        SolidColor,

        LinearGradient,

        RadialGradient,

        Image,

        ClipUpdate, // Update the clip buffer instead of drawing to the framebuffer.
    };
}
