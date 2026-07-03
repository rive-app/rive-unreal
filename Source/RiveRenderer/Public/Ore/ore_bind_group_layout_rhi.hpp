/*
 * Copyright 2025 Rive
 */

#pragma once

#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_bind_group_layout.hpp"
THIRD_PARTY_INCLUDES_END

class OreContextRHI;

namespace rive::ore
{

class OreBindGroupLayoutRHI
    : public LITE_RTTI_OVERRIDE(BindGroupLayout, OreBindGroupLayoutRHI)
{
public:
    OreBindGroupLayoutRHI(OreContextRHI* Context,
                          const rive::ore::BindGroupLayoutDesc& desc);
    virtual ~OreBindGroupLayoutRHI() = default;
};

} // namespace rive::ore
