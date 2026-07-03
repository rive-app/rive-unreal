/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_buffer_rhi.hpp"

#include "RHICommandList.h"
#include "Logs/RiveRendererLog.h"

namespace rive::ore
{

OreBufferRHI::OreBufferRHI(uint32_t size, BufferUsage usage) :
    lite_rtti_override(size, usage)
{
    // Uniform / upload buffers keep a CPU shadow so setBindGroup can
    // snapshot (offset, size) slices into single-draw uniform buffers.
    if (usage == BufferUsage::uniform || usage == BufferUsage::upload)
    {
        m_shadow.resize(size, 0);
    }
}

void OreBufferRHI::update(const void* data, uint32_t size, uint32_t offset)
{
    if (!m_shadow.empty() && m_usage == BufferUsage::uniform)
    {
        check(offset + size <= m_shadow.size());
        memcpy(m_shadow.data() + offset, data, size);
    }
    else
    {
        if (!m_buffer.IsValid())
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("OreBufferRHI::update invalid buffer"));
            return;
        }

        auto& RHICmdList = GRHICommandList.GetImmediateCommandList();
        void* DestPtr = RHICmdList.LockBuffer(m_buffer,
                                              offset,
                                              size,
                                              EResourceLockMode::RLM_WriteOnly);
        memcpy(DestPtr, data, size);
        RHICmdList.UnlockBuffer(m_buffer);
    }
}

} // namespace rive::ore
