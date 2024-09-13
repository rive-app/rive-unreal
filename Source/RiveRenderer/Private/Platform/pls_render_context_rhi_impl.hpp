#pragma once
#include <chrono>

#include "Shaders/ShaderPipelineManager.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/image.hpp"
#include "rive/renderer/buffer_ring.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
namespace gpu
{
class RIVERENDERER_API RenderTargetRHI : public RenderTarget
{
public:
    RenderTargetRHI(FRHICommandList& RHICmdList, const FTexture2DRHIRef& InTextureTarget);
    virtual ~RenderTargetRHI() override {}
    
    FTexture2DRHIRef texture()const
    {return m_textureTarget;}

    FUnorderedAccessViewRHIRef targetUAV()const
    {return m_targetUAV;}

    FUnorderedAccessViewRHIRef coverageUAV()const
    {return m_coverageUAV;}

    FUnorderedAccessViewRHIRef clipUAV()const
    {return m_clipUAV;}

    FUnorderedAccessViewRHIRef scratchColorUAV()const
    {return m_scratchColorUAV;}
    
private:
    FTexture2DRHIRef m_scratchColorTexture;
    FTexture2DRHIRef m_textureTarget;
    FTexture2DRHIRef m_atomicCoverageTexture;
    FTexture2DRHIRef m_clipTexture;

    FUnorderedAccessViewRHIRef m_coverageUAV;
    FUnorderedAccessViewRHIRef m_clipUAV;
    FUnorderedAccessViewRHIRef m_scratchColorUAV;
    FUnorderedAccessViewRHIRef m_targetUAV;
};

class StructuredBufferRingRHIImpl;

class BufferRingRHIImpl final : public BufferRing
{
public:
    BufferRingRHIImpl(EBufferUsageFlags flags, size_t in_sizeInBytes, size_t stride);
    void Sync(FRHICommandList& commandList) const;
    FBufferRHIRef contents()const;
    
protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    virtual void onUnmapAndSubmitBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    
private:
    FBufferRHIRef m_buffer;
    EBufferUsageFlags m_flags;
};

template<typename UniformBufferType>
class UniformBufferRHIImpl : public BufferRing
{
public:
    UniformBufferRHIImpl(size_t in_sizeInBytes): BufferRing(in_sizeInBytes)
    {
        //m_buffer =  TUniformBufferRef<UniformBufferType>::CreateEmptyUniformBufferImmediate( UniformBuffer_MultiFrame);
    }

    void Sync(FRHICommandList& commandList, int offset)
    {
        UniformBufferType* Buffer = reinterpret_cast<UniformBufferType*>(shadowBuffer() + offset);
        m_buffer = TUniformBufferRef<UniformBufferType>::CreateUniformBufferImmediate( *Buffer,UniformBuffer_SingleFrame);
    }

    TUniformBufferRef<UniformBufferType> contents() const
    {
        return m_buffer;
    }

protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override
    {
        return shadowBuffer();
    }

    virtual void onUnmapAndSubmitBuffer(int bufferIdx, size_t mapSizeInBytes) override
    {
    }

private:
    TUniformBufferRef<UniformBufferType> m_buffer;
};

class RenderBufferRHIImpl final: public lite_rtti_override<RenderBuffer, RenderBufferRHIImpl>
{
public:
    RenderBufferRHIImpl(RenderBufferType in_type,
    RenderBufferFlags in_flags, size_t in_sizeInBytes, size_t stride);
    void Sync(FRHICommandList& commandList) const;
    FBufferRHIRef contents()const;
    
protected:
    virtual void* onMap()override;
    virtual void onUnmap()override;
  
    
private:
    BufferRingRHIImpl m_buffer;
    void* m_mappedBuffer;
};

class StructuredBufferRingRHIImpl final : public BufferRing
{
public:
    StructuredBufferRingRHIImpl(EBufferUsageFlags flags, size_t in_sizeInBytes, size_t elementSize);
    template<typename HighLevelStruct>
    void Sync(FRHICommandList& commandList, size_t elementOffset, size_t elementCount)
    {
        auto data = commandList.LockBuffer(m_buffer, 0, elementCount * sizeof(HighLevelStruct), RLM_WriteOnly_NoOverwrite);
        memcpy(data, shadowBuffer() + (elementOffset * sizeof(HighLevelStruct)), elementCount * sizeof(HighLevelStruct));
        commandList.UnlockBuffer(m_buffer);
    }
    FBufferRHIRef contents()const;
    FShaderResourceViewRHIRef srv() const;

protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    virtual void onUnmapAndSubmitBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    
private:
    FBufferRHIRef m_buffer;
    EBufferUsageFlags m_flags;
    FShaderResourceViewRHIRef m_srv;
    size_t m_elementSize;
    size_t m_lastMapSizeInBytes;
};

enum class EVertexDeclarations : int32
{
    Tessellation,
    Gradient,
    Paths,
    InteriorTriangles,
    ImageRect,
    ImageMesh,
    Resolve,
    NumVertexDeclarations
};
    
class RIVERENDERER_API RenderContextRHIImpl : public RenderContextImpl
{
public:
    static std::unique_ptr<RenderContext> MakeContext(FRHICommandListImmediate& CommandListImmediate);
    
    RenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate);

    rcp<RenderTargetRHI> makeRenderTarget(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTargetTexture);

    virtual double secondsNow() const override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_localEpoch;
        return std::chrono::duration<double>(elapsed).count();
    }

    virtual rcp<Texture> decodeImageTexture(Span<const uint8_t> encodedBytes) override;

    virtual void resizeFlushUniformBuffer(size_t sizeInBytes) override;
    virtual void resizeImageDrawUniformBuffer(size_t sizeInBytes) override;
    virtual void resizePathBuffer(size_t sizeInBytes, StorageBufferStructure) override;
    virtual void resizePaintBuffer(size_t sizeInBytes, StorageBufferStructure) override;
    virtual void resizePaintAuxBuffer(size_t sizeInBytes, StorageBufferStructure) override;
    virtual void resizeContourBuffer(size_t sizeInBytes, StorageBufferStructure) override;
    virtual void resizeSimpleColorRampsBuffer(size_t sizeInBytes) override;
    virtual void resizeGradSpanBuffer(size_t sizeInBytes) override;
    virtual void resizeTessVertexSpanBuffer(size_t sizeInBytes) override;
    virtual void resizeTriangleVertexBuffer(size_t sizeInBytes) override;

    virtual void* mapFlushUniformBuffer(size_t mapSizeInBytes) override;
    virtual void* mapImageDrawUniformBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPathBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPaintBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPaintAuxBuffer(size_t mapSizeInBytes) override;
    virtual void* mapContourBuffer(size_t mapSizeInBytes) override;
    virtual void* mapSimpleColorRampsBuffer(size_t mapSizeInBytes) override;
    virtual void* mapGradSpanBuffer(size_t mapSizeInBytes) override;
    virtual void* mapTessVertexSpanBuffer(size_t mapSizeInBytes) override;
    virtual void* mapTriangleVertexBuffer(size_t mapSizeInBytes) override;

    virtual void unmapFlushUniformBuffer() override;
    virtual void unmapImageDrawUniformBuffer() override;
    virtual void unmapPathBuffer() override;
    virtual void unmapPaintBuffer() override;
    virtual void unmapPaintAuxBuffer() override;
    virtual void unmapContourBuffer() override;
    virtual void unmapSimpleColorRampsBuffer() override;
    virtual void unmapGradSpanBuffer() override;
    virtual void unmapTessVertexSpanBuffer() override;
    virtual void unmapTriangleVertexBuffer() override;
    
    virtual rcp<RenderBuffer> makeRenderBuffer(RenderBufferType, RenderBufferFlags, size_t) override;
    virtual void resizeGradientTexture(uint32_t width, uint32_t height) override;
    virtual void resizeTessellationTexture(uint32_t width, uint32_t height) override;
    
    virtual void flush(const FlushDescriptor&)override;
    
private:
    FTextureRHIRef m_gradiantTexture;
    FTextureRHIRef m_tesselationTexture;

    FBufferRHIRef m_patchVertexBuffer;
    FBufferRHIRef m_patchIndexBuffer;
    FBufferRHIRef m_imageRectVertexBuffer;
    FBufferRHIRef m_imageRectIndexBuffer;
    FBufferRHIRef m_tessSpanIndexBuffer;

    FShaderResourceViewRHIRef m_tessSRV;
    
    FSamplerStateRHIRef m_linearSampler;
    FSamplerStateRHIRef m_mipmapSampler;

    FRHIVertexDeclaration* VertexDeclarations[static_cast<int32>(EVertexDeclarations::NumVertexDeclarations)];
    
    std::unique_ptr<UniformBufferRHIImpl<FFlushUniforms>> m_flushUniformBuffer;
    std::unique_ptr<UniformBufferRHIImpl<FImageDrawUniforms>> m_imageDrawUniformBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_pathBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_paintBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_paintAuxBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_contourBuffer;
    std::unique_ptr<HeapBufferRing>    m_simpleColorRampsBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_gradSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_tessSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_triangleBuffer;
    
    std::chrono::steady_clock::time_point m_localEpoch = std::chrono::steady_clock::now();
};
} // namespace pls
} // namespace rive