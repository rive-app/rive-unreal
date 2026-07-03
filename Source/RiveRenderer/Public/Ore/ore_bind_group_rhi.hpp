/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RHI.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_bind_group.hpp"
#include "rive/renderer/ore/ore_binding_map.hpp"
THIRD_PARTY_INCLUDES_END

#include <vector>

class OreContextRHI;

namespace rive::ore
{
class OreRenderPassRHI;
class OreBufferRHI;

class OreBindGroupRHI : public LITE_RTTI_OVERRIDE(BindGroup, OreBindGroupRHI)
{
public:
    // UE's RHI binds loose resources per-stage by register slot — the same
    // model as D3D11's independent VS / PS namespaces — so each binding
    // carries the native slot pre-resolved for each stage at makeBindGroup
    // time. kAbsent means the resource is not visible to that stage.
    struct RHIUBOBinding
    {
        OreBufferRHI* buffer =
            nullptr;          // raw ptr; m_retainedBuffers keeps it alive
        uint32_t binding = 0; // WGSL @binding, for sort
        uint16_t vsSlot = BindingMap::kAbsent;
        uint16_t fsSlot = BindingMap::kAbsent;
        uint32_t offset = 0;
        uint32_t size = 0;
        bool hasDynamicOffset = false;
    };
    struct RHITexBinding
    {
        // Null when the view wraps an external texture (wrapCanvasTexture /
        // wrapRiveTexture) that never went through makeTextureView; bind
        // rhiTexture directly in that case.
        FRHIShaderResourceView* srv = nullptr;
        FRHITexture* rhiTexture = nullptr;
        uint16_t vsSlot = BindingMap::kAbsent;
        uint16_t fsSlot = BindingMap::kAbsent;
    };
    struct RHISamplerBinding
    {
        FRHISamplerState* rhiSampler = nullptr;
        uint16_t vsSlot = BindingMap::kAbsent;
        uint16_t fsSlot = BindingMap::kAbsent;
    };

    OreBindGroupRHI() = default;
    virtual ~OreBindGroupRHI() = default;

private:
    friend class OreRenderPassRHI;
    friend class ::OreContextRHI;

    std::vector<RHIUBOBinding> m_ubos;
    std::vector<RHITexBinding> m_textures;
    std::vector<RHISamplerBinding> m_samplers;
};

} // namespace rive::ore
