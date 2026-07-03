/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RHI.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_buffer.hpp"
THIRD_PARTY_INCLUDES_END

#include <vector>

namespace rive::ore
{

class OreBufferRHI : public LITE_RTTI_OVERRIDE(Buffer, OreBufferRHI)
{
public:
    OreBufferRHI(uint32_t size, BufferUsage usage);
    virtual ~OreBufferRHI() = default;

    virtual void update(const void* data,
                        uint32_t size,
                        uint32_t offset = 0) override;

    FBufferRHIRef m_buffer;

    // CPU shadow of the buffer contents, kept for uniform / upload usage
    // only. UE has no offset-carrying uniform-buffer bind (no
    // VSSetConstantBuffers1 equivalent), so OreRenderPassRHI::ResolveUBO
    // snapshots the bound slice out of this shadow into a single-draw
    // FRHIUniformBuffer at setBindGroup time.
    std::vector<uint8_t> m_shadow;
};

} // namespace rive::ore
