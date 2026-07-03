/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RHI.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_sampler.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::ore
{

class OreSamplerRHI : public LITE_RTTI_OVERRIDE(Sampler, OreSamplerRHI)
{
public:
    OreSamplerRHI();
    virtual ~OreSamplerRHI() = default;

    FSamplerStateRHIRef m_sampler;
};

} // namespace rive::ore
