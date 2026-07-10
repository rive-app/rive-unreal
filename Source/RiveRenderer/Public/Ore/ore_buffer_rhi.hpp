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

class FRHICommandList;

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

    // Index buffers only: returns m_buffer, recreating it (from the CPU
    // shadow) with the given stride first if the current stride differs. UE
    // derives the 16- vs 32-bit index format from the buffer's stride — there
    // is no per-bind index format like the native backends' — but Ore only
    // reveals the format at setIndexBuffer time, after the buffer exists.
    FRHIBuffer* indexBufferWithStride(FRHICommandList& RHICmdList,
                                      uint32 stride);

    FBufferRHIRef m_buffer;

    // The EBufferUsageFlags m_buffer was created with, so
    // indexBufferWithStride can recreate it faithfully.
    EBufferUsageFlags m_usageFlags = EBufferUsageFlags::None;

    // CPU shadow of the buffer contents, kept for uniform / upload / index
    // usage. UE has no offset-carrying uniform-buffer bind (no
    // VSSetConstantBuffers1 equivalent), so OreRenderPassRHI::ResolveUBO
    // snapshots the bound slice out of this shadow into a single-draw
    // FRHIUniformBuffer at setBindGroup time. For index buffers it is the
    // data source when indexBufferWithStride has to recreate the RHI buffer.
    std::vector<uint8_t> m_shadow;
};

} // namespace rive::ore
