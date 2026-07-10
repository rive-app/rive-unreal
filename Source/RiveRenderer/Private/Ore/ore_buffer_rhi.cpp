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
    // Index buffers keep one so indexBufferWithStride can recreate the RHI
    // buffer when the bound index format's stride differs from the stride it
    // was created with.
    if (usage == BufferUsage::uniform || usage == BufferUsage::upload ||
        usage == BufferUsage::index)
    {
        m_shadow.resize(size, 0);
    }
}

void OreBufferRHI::update(const void* data, uint32_t size, uint32_t offset)
{
    // Mirror into the CPU shadow when one exists.
    if (!m_shadow.empty())
    {
        check(offset + size <= m_shadow.size());
        memcpy(m_shadow.data() + offset, data, size);
    }

    // Uniform buffers have no RHI buffer. ResolveUBO materializes slices of
    // the shadow into single-draw uniform buffers instead.
    if (m_usage == BufferUsage::uniform)
    {
        return;
    }

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

FRHIBuffer* OreBufferRHI::indexBufferWithStride(FRHICommandList& RHICmdList,
                                                uint32 stride)
{
    check(m_usage == BufferUsage::index);
    if (m_buffer.IsValid() && m_buffer->GetStride() == stride)
    {
        return m_buffer;
    }

    // The bound index format's stride doesn't match the buffer, so UE's draw
    // calls would decode the indices with the wrong width. Recreate at the
    // requested stride and refill from the CPU shadow. Draws already recorded
    // keep the old buffer alive via their own reference.
    FRHIBufferCreateDesc Desc =
        FRHIBufferCreateDesc::Create(TEXT("OreIndexBuffer"),
                                     m_size,
                                     stride,
                                     m_usageFlags);
    Desc.DetermineInitialState();
    FBufferRHIRef NewBuffer = RHICmdList.CreateBuffer(Desc);

    void* DestPtr = RHICmdList.LockBuffer(NewBuffer,
                                          0,
                                          m_size,
                                          EResourceLockMode::RLM_WriteOnly);
    memcpy(DestPtr, m_shadow.data(), m_size);
    RHICmdList.UnlockBuffer(NewBuffer);

    m_buffer = NewBuffer;
    return m_buffer;
}

} // namespace rive::ore
