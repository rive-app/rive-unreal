#pragma once
#include <chrono>

#include "Logs/RiveRendererLog.h"
#include "RiveShaders/Public/RiveShaderTypes.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include "rive/renderer/buffer_ring.hpp"
THIRD_PARTY_INCLUDES_END

struct RHICapabilities
{
    RHICapabilities();

    bool bSupportsPixelShaderUAVs = false;
    bool bSupportsTypedUAVLoads = false;
    bool bSupportsRasterOrderViews = false;
};

class RIVERENDERER_API RenderTargetRHI : public rive::gpu::RenderTarget
{
public:
    RenderTargetRHI(FRHICommandList& RHICmdList,
                    const RHICapabilities& Capabilities,
                    const FTexture2DRHIRef& InTextureTarget);
    virtual ~RenderTargetRHI() override {}

    FTexture2DRHIRef texture() const { return m_textureTarget; }

    FTexture2DRHIRef clipTexture() const { return m_clipTexture; }

    FTexture2DRHIRef coverageTexture() const { return m_atomicCoverageTexture; }

    FUnorderedAccessViewRHIRef targetUAV() const { return m_targetUAV; }

    FUnorderedAccessViewRHIRef coverageUAV() const { return m_coverageUAV; }

    FUnorderedAccessViewRHIRef clipUAV() const { return m_clipUAV; }

    FUnorderedAccessViewRHIRef scratchColorUAV() const
    {
        return m_scratchColorUAV;
    }

    bool TargetTextureSupportsUAV() const { return m_targetTextureSupportsUAV; }

private:
    FTexture2DRHIRef m_scratchColorTexture;
    FTexture2DRHIRef m_textureTarget;
    FTexture2DRHIRef m_atomicCoverageTexture;
    FTexture2DRHIRef m_clipTexture;

    FUnorderedAccessViewRHIRef m_coverageUAV;
    FUnorderedAccessViewRHIRef m_clipUAV;
    FUnorderedAccessViewRHIRef m_scratchColorUAV;
    FUnorderedAccessViewRHIRef m_targetUAV;

    bool m_targetTextureSupportsUAV;
};

// TODO: Make this use staging buffer
class BufferRingRHIImpl final : public rive::gpu::BufferRing
{
public:
    BufferRingRHIImpl(EBufferUsageFlags flags,
                      size_t InSizeInBytes,
                      size_t stride);
    FBufferRHIRef Sync(FRHICommandList& commandList,
                       size_t offsetInBytes = 0) const;

protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    virtual void onUnmapAndSubmitBuffer(int bufferIdx,
                                        size_t mapSizeInBytes) override;

private:
    EBufferUsageFlags m_flags;
    size_t m_stride;
};

template <typename UniformBufferType>
class UniformBufferRHIImpl : public rive::gpu::BufferRing
{
public:
    UniformBufferRHIImpl(size_t InSizeInBytes) : BufferRing(InSizeInBytes) {}

    void Sync(FRHICommandList& commandList, size_t offset)
    {
        UniformBufferType* Buffer =
            reinterpret_cast<UniformBufferType*>(shadowBuffer() + offset);
        m_buffer =
            TUniformBufferRef<UniformBufferType>::CreateUniformBufferImmediate(
                *Buffer,
                UniformBuffer_SingleFrame);
    }

    TUniformBufferRef<UniformBufferType> contents() const { return m_buffer; }

protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override
    {
        return shadowBuffer();
    }

    virtual void onUnmapAndSubmitBuffer(int bufferIdx,
                                        size_t mapSizeInBytes) override
    {}

private:
    TUniformBufferRef<UniformBufferType> m_buffer;
};

class RenderBufferRHIImpl final
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderBuffer, RenderBufferRHIImpl)
{
public:
    RenderBufferRHIImpl(rive::RenderBufferType inType,
                        rive::RenderBufferFlags inFlags,
                        size_t inSizeInBytes,
                        size_t stride);
    FBufferRHIRef Sync(FRHICommandList& commandList) const;

protected:
    virtual void* onMap() override;
    virtual void onUnmap() override;

private:
    BufferRingRHIImpl m_buffer;
    void* m_mappedBuffer;
};

template <typename HighLevelStruct> class StructuredBufferRHIImpl final
{
public:
    StructuredBufferRHIImpl(EBufferUsageFlags flags) :
        m_flags(flags), m_sizeInBytes(0), m_lastMapSizeInBytes(0), m_data(true)
    {}

    StructuredBufferRHIImpl() :
        m_flags(EBufferUsageFlags::ShaderResource),
        m_sizeInBytes(0),
        m_lastMapSizeInBytes(0),
        m_data(true)
    {}

    void Resize(size_t newSizeInBytes, size_t gpuStride)
    {
        check(m_gpuStride == gpuStride)
            m_data.SetNumUninitialized(newSizeInBytes / m_cpuStride);
        m_sizeInBytes = newSizeInBytes;
    }

    FBufferRHIRef Sync(FRHICommandList& commandList)
    {
        if (m_sizeInBytes == 0)
            return nullptr;

        FRHIResourceCreateInfo Info(TEXT("StructuredBufferRHIImpl"), &m_data);
        return commandList.CreateStructuredBuffer(
            m_gpuStride,
            m_data.GetResourceDataSize(),
            m_flags | EBufferUsageFlags::Volatile,
            Info);
    }

    FShaderResourceViewRHIRef SyncSRV(FRHICommandList& commandList,
                                      size_t elementOffset,
                                      size_t elementCount)
    {
        if (auto buffer = Sync(commandList))
        {
            return commandList.CreateShaderResourceView(
                buffer,
                FRHIViewDesc::CreateBufferSRV()
                    .SetType(FRHIViewDesc::EBufferType::Structured)
                    .SetOffsetInBytes(elementOffset * m_cpuStride)
                    .SetNumElements(elementCount *
                                    (m_cpuStride / m_gpuStride)));
        }

        return nullptr;
    }

    FShaderResourceViewRHIRef SyncSRV(FRHICommandList& commandList)
    {
        auto buffer = Sync(commandList);
        return commandList.CreateShaderResourceView(
            buffer,
            FRHIViewDesc::CreateBufferSRV().SetType(
                FRHIViewDesc::EBufferType::Structured));
    }

    void* Map(size_t mapSizeInBytes)
    {
        // we cant be 0 size
        check(m_sizeInBytes);
        // our map size must be less than or equal to our actual size
        check(mapSizeInBytes <= m_sizeInBytes);
        // our stride must be a factor of our map size
        check(mapSizeInBytes % m_gpuStride == 0);
        m_lastMapSizeInBytes = mapSizeInBytes;
        return m_data.GetData();
    }

    size_t GPUSize() const { return m_sizeInBytes / m_gpuStride; }

private:
    EBufferUsageFlags m_flags;
    size_t m_sizeInBytes;
    size_t m_lastMapSizeInBytes;
    TResourceArray<HighLevelStruct> m_data;

    static constexpr size_t m_cpuStride = sizeof(HighLevelStruct);
    static constexpr size_t m_gpuStride =
        rive::gpu::StorageBufferElementSizeInBytes(
            HighLevelStruct::kBufferStructure);
};

class DelayLoadedTexture
{
public:
    DelayLoadedTexture() {}
    void UpdateTexture(const FRHITextureCreateDesc& inDesc,
                       bool inNeedsSRV = false);
    void Sync(FRHICommandList& RHICmdList);

    FTextureRHIRef Contents() const { return m_texture; }

    FShaderResourceViewRHIRef SRV() const { return m_srv; }

private:
    FTextureRHIRef m_texture;
    FShaderResourceViewRHIRef m_srv;
    FRHITextureCreateDesc m_desc;
    bool m_isDirty = false;
    bool m_needsSRV = false;
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

class RIVERENDERER_API RenderContextRHIImpl
    : public rive::gpu::RenderContextImpl
{
public:
    static std::unique_ptr<rive::gpu::RenderContext> MakeContext(
        FRHICommandListImmediate& CommandListImmediate);

    RenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate);

    rive::rcp<RenderTargetRHI> makeRenderTarget(
        FRHICommandListImmediate& RHICmdList,
        const FTexture2DRHIRef& InTargetTexture);

    virtual double secondsNow() const override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_localEpoch;
        return std::chrono::duration<double>(elapsed).count();
    }

    virtual rive::rcp<rive::gpu::Texture> decodeImageTexture(
        rive::Span<const uint8_t> encodedBytes) override;

    virtual void resizeFlushUniformBuffer(size_t sizeInBytes) override;
    virtual void resizeImageDrawUniformBuffer(size_t sizeInBytes) override;
    virtual void resizePathBuffer(size_t sizeInBytes,
                                  rive::gpu::StorageBufferStructure) override;
    virtual void resizePaintBuffer(size_t sizeInBytes,
                                   rive::gpu::StorageBufferStructure) override;
    virtual void resizePaintAuxBuffer(
        size_t sizeInBytes,
        rive::gpu::StorageBufferStructure) override;
    virtual void resizeContourBuffer(
        size_t sizeInBytes,
        rive::gpu::StorageBufferStructure) override;
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

    virtual rive::rcp<rive::RenderBuffer> makeRenderBuffer(
        rive::RenderBufferType,
        rive::RenderBufferFlags,
        size_t) override;
    virtual void resizeGradientTexture(uint32_t width,
                                       uint32_t height) override;
    virtual void resizeTessellationTexture(uint32_t width,
                                           uint32_t height) override;

    virtual void flush(const rive::gpu::FlushDescriptor&) override;

private:
    DelayLoadedTexture m_gradiantTexture;
    DelayLoadedTexture m_tesselationTexture;

    FBufferRHIRef m_patchVertexBuffer;
    FBufferRHIRef m_patchIndexBuffer;
    FBufferRHIRef m_imageRectVertexBuffer;
    FBufferRHIRef m_imageRectIndexBuffer;
    FBufferRHIRef m_tessSpanIndexBuffer;

    FSamplerStateRHIRef m_linearSampler;
    FSamplerStateRHIRef m_mipmapSampler;

    FRHIVertexDeclaration* VertexDeclarations[static_cast<int32>(
        EVertexDeclarations::NumVertexDeclarations)];

    std::unique_ptr<UniformBufferRHIImpl<FFlushUniforms>> m_flushUniformBuffer;
    std::unique_ptr<UniformBufferRHIImpl<FImageDrawUniforms>>
        m_imageDrawUniformBuffer;
    StructuredBufferRHIImpl<rive::gpu::PathData> m_pathBuffer;
    StructuredBufferRHIImpl<rive::gpu::PaintData> m_paintBuffer;
    StructuredBufferRHIImpl<rive::gpu::PaintAuxData> m_paintAuxBuffer;
    StructuredBufferRHIImpl<rive::gpu::ContourData> m_contourBuffer;
    std::unique_ptr<rive::gpu::HeapBufferRing> m_simpleColorRampsBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_gradSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_tessSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_triangleBuffer;

    std::chrono::steady_clock::time_point m_localEpoch =
        std::chrono::steady_clock::now();

    RHICapabilities m_capabilities;
};
