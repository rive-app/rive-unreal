#pragma once
#include "Shaders/ShaderPipelineManager.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include <rive/pls/pls_render_context_helper_impl.hpp>
#include "rive/pls/pls_image.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
namespace pls
{
class PLSRenderTargetRHI : public PLSRenderTarget
{
public:
    PLSRenderTargetRHI(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTextureTarget);
    virtual ~PLSRenderTargetRHI() override {}

    FShaderResourceViewRHIRef targetSRV() const
    { return m_targetSRV; }
    
    FTexture2DRHIRef texture()const
    {return m_textureTarget;}

    FUnorderedAccessViewRHIRef targetUAV()const
    {return m_targetUAV;}

    FUnorderedAccessViewRHIRef coverageUAV()const
    {return m_coverageUAV;}

    FUnorderedAccessViewRHIRef scratchColorUAV()const
    {return m_scratchColorUAV;}
    
private:
    FTexture2DRHIRef m_scratchColorTexture;
    FTexture2DRHIRef m_textureTarget;
    FTexture2DRHIRef m_atomicCoverageTexture;

    FUnorderedAccessViewRHIRef m_coverageUAV;
    FUnorderedAccessViewRHIRef m_scratchColorUAV;
    FUnorderedAccessViewRHIRef m_targetUAV;
    FShaderResourceViewRHIRef m_targetSRV;
};

class StructuredBufferRingRHIImpl;

class BufferRingRHIImpl final : public BufferRing
{
public:
    BufferRingRHIImpl(EBufferUsageFlags flags, size_t in_sizeInBytes);
    void Sync(FRHICommandListImmediate& commandList) const;
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

    void Sync(FRHICommandListImmediate& commandList, int offset)
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
    RenderBufferFlags in_flags, size_t in_sizeInBytes);
    void Sync(FRHICommandListImmediate& commandList) const;
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
    void Sync(FRHICommandListImmediate& commandList) const;
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
};

class PLSRenderContextRHIImpl : public PLSRenderContextImpl
{
public:
    static std::unique_ptr<PLSRenderContext> MakeContext(FRHICommandListImmediate& CommandListImmediate);
    
    PLSRenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate);

    rcp<PLSRenderTargetRHI> makeRenderTarget(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTargetTexture);

    virtual double secondsNow() const override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_localEpoch;
        return std::chrono::duration<double>(elapsed).count();
    }

    virtual rcp<PLSTexture> decodeImageTexture(Span<const uint8_t> encodedBytes) override;

    virtual void resizeFlushUniformBuffer(size_t sizeInBytes) override;
    virtual void resizeImageDrawUniformBuffer(size_t sizeInBytes) override;
    virtual void resizePathBuffer(size_t sizeInBytes, pls::StorageBufferStructure) override;
    virtual void resizePaintBuffer(size_t sizeInBytes, pls::StorageBufferStructure) override;
    virtual void resizePaintAuxBuffer(size_t sizeInBytes, pls::StorageBufferStructure) override;
    virtual void resizeContourBuffer(size_t sizeInBytes, pls::StorageBufferStructure) override;
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
    
    virtual void flush(const pls::FlushDescriptor&)override;
    
private:
    FTextureRHIRef m_gradiantTexture;
    FTextureRHIRef m_tesselationTexture;

    FSamplerStateRHIRef m_linearSampler;
    FSamplerStateRHIRef m_mipmapSampler;
    
    std::unique_ptr<ImageMeshPipeline> m_imageMeshPipeline;
    std::unique_ptr<GradientPipeline> m_gradientPipeline;
    std::unique_ptr<TessPipeline> m_tessPipeline;
    std::unique_ptr<AtomicResolvePipeline> m_atomicResolvePipeline;
    std::unique_ptr<TestSimplePipeline> m_testSimplePipeline;
    
    std::unique_ptr<UniformBufferRHIImpl<FFlushUniforms>> m_flushUniformBuffer;
    std::unique_ptr<UniformBufferRHIImpl<FImageDrawUniforms>> m_imageDrawUniformBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_pathBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_paintBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_paintAuxBuffer;
    std::unique_ptr<StructuredBufferRingRHIImpl> m_contourBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_simpleColorRampsBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_gradSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_tessSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_triangleBuffer;
    
    FBufferRHIRef m_tessSpanIndexBuffer;
    TResourceArray<uint16_t> m_tessSpanIndexData;
    
    std::chrono::steady_clock::time_point m_localEpoch = std::chrono::steady_clock::now();
};
} // namespace pls
} // namespace rive