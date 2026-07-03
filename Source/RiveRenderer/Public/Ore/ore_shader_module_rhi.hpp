/*
 * Copyright 2025 Rive
 */

#pragma once

#include "Shader.h"
#include "RHIResources.h"
#include "Ore/OreSlotRemap.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_shader_module.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::ore
{

class OreShaderModuleRHI
    : public LITE_RTTI_OVERRIDE(ShaderModule, OreShaderModuleRHI)
{
public:
    OreShaderModuleRHI() = default;
    virtual ~OreShaderModuleRHI() = default;

    // Exactly one is set, per the module's stage. Created directly from the
    // captured bytecode blob in OreContextRHI::makeShaderModule — no FShader or
    // global shader map involved.
    FVertexShaderRHIRef VertexShader;
    FPixelShaderRHIRef PixelShader;

    // Vulkan register->bind-index remap for this stage (empty on D3D/Metal).
    // Copied from the cooked/compiled blob in makeShaderModule; flows into the
    // pipeline and is applied per-bind in OreRenderPassRHI::setBindGroup.
    FOreVulkanSlotRemap SlotRemap;
};

} // namespace rive::ore
