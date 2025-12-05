// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once
#include <chrono>

#include "RenderGraphBuilder.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Logs/RiveRendererLog.h"
#include <RiveShaders/Public/RiveShaderTypes.h>
#include "UnrealClient.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/shapes/paint/image_sampler.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include "rive/renderer/buffer_ring.hpp"
THIRD_PARTY_INCLUDES_END

struct RHICapabilities
{
    RHICapabilities();

    bool bSupportsPixelShaderUAVs = true;
    bool bSupportsTypedUAVLoads = false;
    bool bSupportsRasterOrderViews = false;

    FString AsString() const;
};

class RIVERENDERER_API RenderTargetRHI : public rive::gpu::RenderTarget
{
public:
    RenderTargetRHI(FRHICommandList& RHICmdList,
                    const RHICapabilities& Capabilities,
                    const FTextureRHIRef& InTextureTarget);

    RenderTargetRHI(FRHICommandList& RHICmdList,
                    const RHICapabilities& Capabilities,
                    FRenderTarget* InTextureTarget);

    RenderTargetRHI(FRDGBuilder& GraphBuilder,
                    const RHICapabilities& Capabilities,
                    FRDGTextureRef InTextureTarget);

    virtual ~RenderTargetRHI() override {}

    // we don't want to re-create the backing textures to this every frame, so
    // instead just add the ability to update the target texture instead and
    // only update the backing texture if the sizes change
    void updateTargetTexture(FRDGTextureRef InTextureTarget)
    {
        check(InTextureTarget->Desc.Extent.X == width() &&
              InTextureTarget->Desc.Extent.Y == height());
        m_rdgTextureTarget = InTextureTarget;
        m_targetTextureSupportsUAV = static_cast<bool>(
            m_rdgTextureTarget->Desc.Flags & ETextureCreateFlags::UAV);

        m_targetTextureSupportsRenderTarget =
            static_cast<bool>(m_rdgTextureTarget->Desc.Flags &
                              ETextureCreateFlags::RenderTargetable);

        m_targetTextureSupportsResolveTarget =
            static_cast<bool>(m_rdgTextureTarget->Desc.Flags &
                              ETextureCreateFlags::ResolveTargetable);
    }

    void updateTargetTexture(FRenderTarget* InTextureTarget)
    {
        check(InTextureTarget->GetSizeXY().X == width() &&
              InTextureTarget->GetSizeXY().Y == height());
        m_renderTarget = InTextureTarget;
        const auto rhiTexture = m_renderTarget->GetRenderTargetTexture();
        m_targetTextureSupportsUAV = static_cast<bool>(
            rhiTexture->GetDesc().Flags & ETextureCreateFlags::UAV);

        m_targetTextureSupportsRenderTarget =
            static_cast<bool>(rhiTexture->GetDesc().Flags &
                              ETextureCreateFlags::RenderTargetable);

        m_targetTextureSupportsResolveTarget =
            static_cast<bool>(rhiTexture->GetDesc().Flags &
                              ETextureCreateFlags::ResolveTargetable);
    }

    // RDG Interface, RDG objects can not be cached so register the RHI textures
    // as "external resources" instead and return that per logic flush / Graph
    // Builder instance

    FRDGTextureRef targetTexture(FRDGBuilder& Builder);

    // Used when the target texture does not support UAVs
    FRDGTextureRef backBufferTexture(FRDGBuilder& Builder,
                                     const FLinearColor& ClearColor,
                                     const rive::gpu::FlushDescriptor& desc);

    FRDGTextureRef clipTexture(FRDGBuilder& Builder);
#if PLATFORM_APPLE
    FRDGBufferRef coverageBuffer(FRDGBuilder& Builder);
#endif
    FRDGTextureRef coverageTexture(FRDGBuilder& Builder);

    FRDGTextureRef msaaColorTexture(FRDGBuilder& Builder,
                                    const FLinearColor& ClearColor);

    FRDGTextureRef msaaStencilTexture(FRDGBuilder& Builder,
                                      float DepthClear,
                                      uint32 StencilClear);

    bool TargetTextureSupportsUAV() const { return m_targetTextureSupportsUAV; }
    bool TargetTextureSupportsRenderTarget() const
    {
        return m_targetTextureSupportsRenderTarget;
    }
    bool TargetTextureSupportsResolveTarget() const
    {
        return m_targetTextureSupportsResolveTarget;
    }

    FTextureRHIRef texture() const { return m_textureTarget; }

private:
    FRenderTarget* m_renderTarget = nullptr;
    FRDGTextureRef m_rdgTextureTarget = nullptr;
    FTextureRHIRef m_textureTarget = nullptr;

    bool m_targetTextureSupportsUAV;
    bool m_targetTextureSupportsRenderTarget;
    bool m_targetTextureSupportsResolveTarget;
    // Reference held for convenience. May be better to just DI it everywhere.
    const RHICapabilities& m_capabilities;
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
    FRDGBufferRef Sync(FRDGBuilder& RDGBuilder, size_t offsetInBytes = 0) const;

protected:
    virtual void* onMapBuffer(int bufferIdx, size_t mapSizeInBytes) override;
    virtual void onUnmapAndSubmitBuffer(int bufferIdx,
                                        size_t mapSizeInBytes) override;

private:
    EBufferUsageFlags m_flags;
    size_t m_stride;
};

template <typename CPUUniformBufferType, typename GPUUniformBufferType>
class UniformBufferRHIImpl
{
public:
    UniformBufferRHIImpl(size_t InSizeInBytes) : m_sizeInBytes(InSizeInBytes)
    {
        m_data.SetNumUninitialized(InSizeInBytes / m_cpuStride);
    }

    FORCENOINLINE TUniformBufferRef<GPUUniformBufferType> Sync(
        FRHICommandList& RHICmdList,
        size_t offset)
    {
        // there should be no remander
        check(offset % m_cpuStride == 0);
        FImageDrawUniforms Uniforms;
        memcpy(&Uniforms, &m_data[offset / m_cpuStride], m_gpuStride);
        if (!m_uniformBufferRHIRef)
        {
            // Lazy create this because we only ever want to use it for msaa.
            m_uniformBufferRHIRef = TUniformBufferRef<GPUUniformBufferType>::
                CreateUniformBufferImmediate(
                    Uniforms,
                    EUniformBufferUsage::UniformBuffer_SingleDraw);
        }
        else
        {
            m_uniformBufferRHIRef.UpdateUniformBufferImmediate(RHICmdList,
                                                               Uniforms);
        }

        return m_uniformBufferRHIRef;
    }

    FORCENOINLINE TRDGUniformBufferRef<GPUUniformBufferType> Sync(
        FRDGBuilder& Builder,
        size_t offset)
    {
        // there should be no remander
        check(offset % m_cpuStride == 0);
        GPUUniformBufferType* Buffer =
            Builder.AllocParameters<GPUUniformBufferType>();
        memcpy(Buffer, &m_data[offset / m_cpuStride], m_gpuStride);
        return Builder.CreateUniformBuffer<GPUUniformBufferType>(Buffer);
    }

    void* mapBuffer(size_t mapSizeInBytes)
    {
        // we cant be 0 size
        check(m_sizeInBytes);
        // our map size must be less than or equal to our actual size
        check(mapSizeInBytes <= m_sizeInBytes);
        return m_data.GetData();
    }

private:
    size_t m_sizeInBytes;
    // Used for msaa mode where we bypass rdg per draw, so this is used for
    // image uniforms
    TUniformBufferRef<GPUUniformBufferType> m_uniformBufferRHIRef;
    TResourceArray<CPUUniformBufferType> m_data;
    static constexpr size_t m_cpuStride = sizeof(CPUUniformBufferType);
    static constexpr size_t m_gpuStride = sizeof(GPUUniformBufferType);
};

class RenderBufferRHIImpl final
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderBuffer, RenderBufferRHIImpl)
{
public:
    RenderBufferRHIImpl(rive::RenderBufferType inType,
                        rive::RenderBufferFlags inFlags,
                        size_t inSizeInBytes,
                        size_t stride);
    FBufferRHIRef Sync(FRHICommandList&) const;

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
        check(m_gpuStride == gpuStride);
        m_data.SetNumUninitialized(newSizeInBytes / m_cpuStride);
        if (newSizeInBytes)
        {
            FMemory::Memset(&m_data[0], 0, newSizeInBytes);
        }
        m_sizeInBytes = newSizeInBytes;
    }

    FRDGBufferSRVRef SyncSRV(FRDGBuilder& Builder,
                             size_t elementOffset,
                             size_t elementCount)
    {
        if (m_sizeInBytes == 0)
            return nullptr;

        auto buffer = Builder.CreateBuffer(
            FRDGBufferDesc::CreateStructuredDesc(
                m_gpuStride,
                elementCount * (m_cpuStride / m_gpuStride)),
            TEXT("rive.StructuredBufferRHIImpl"));

        Builder.QueueBufferUpload(buffer,
                                  &m_data[elementOffset],
                                  m_cpuStride * elementCount,
                                  ERDGInitialDataFlags::None);

        return Builder.CreateSRV(FRDGBufferSRVDesc(buffer));
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
    void UpdateTexture(const FRDGTextureDesc& inDesc,
                       FString DebugName,
                       bool inNeedsSRV = false);

    // We don't hold a reference to the RDGTexture because it must be created
    // each frame so instead we provide a function that returns via params
    // outTexture will always be set but outSRV will only be set if something
    // other than null is passed
    void Sync(FRDGBuilder& RDGBuilder,
              FRDGTextureRef* outTexture,
              FRDGTextureSRVRef* outSRV = nullptr) const;

private:
    // used for render graph interface
    FRDGTextureDesc m_rdgDesc;
    FString m_debugName;
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
    Atlas,
    NumVertexDeclarations
};

class RIVERENDERER_API RenderContextRHIImpl
    : public rive::gpu::RenderContextImpl
{
public:
    struct RHICapabilitiesOverrides
    {
        bool bSupportsPixelShaderUAVs = true;
        bool bSupportsTypedUAVLoads = true;
#if PLATFORM_APPLE
        bool bSupportsRasterOrderViews = true;
#else
        bool bSupportsRasterOrderViews = false;
#endif
    };

    static std::unique_ptr<rive::gpu::RenderContext> MakeContext(
        FRHICommandListImmediate& CommandListImmediate,
        const RHICapabilitiesOverrides& Overrides);

    static rive::rcp<rive::RenderImage> MakeExternalRenderImage(
        const FTextureRHIRef& InTargetTexture);

    RenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate,
                         const RHICapabilitiesOverrides& Overrides);
#if WITH_EDITOR
    void updateFromInterlockCVar(int32 CVar);
#endif
    rive::rcp<RenderTargetRHI> makeRenderTarget(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InTargetTexture);

    rive::rcp<RenderTargetRHI> makeRenderTarget(
        FRHICommandListImmediate& RHICmdList,
        FRenderTarget* InTargetTexture);

    rive::rcp<RenderTargetRHI> makeRenderTarget(FRDGBuilder& GraphBuilder,
                                                FRDGTextureRef InTargetTexture);

    virtual double secondsNow() const override
    {
        auto elapsed = std::chrono::steady_clock::now() - m_localEpoch;
        return std::chrono::duration<double>(elapsed).count();
    }

    virtual rive::rcp<rive::gpu::Texture> platformDecodeImageTexture(
        rive::Span<const uint8_t> encodedBytes) override;

    virtual rive::rcp<rive::gpu::Texture> makeImageTexture(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevelCount,
        const uint8_t imageDataRGBA[]) override;

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
    virtual void resizeGradSpanBuffer(size_t sizeInBytes) override;
    virtual void resizeTessVertexSpanBuffer(size_t sizeInBytes) override;
    virtual void resizeTriangleVertexBuffer(size_t sizeInBytes) override;

    virtual void* mapFlushUniformBuffer(size_t mapSizeInBytes) override;
    virtual void* mapImageDrawUniformBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPathBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPaintBuffer(size_t mapSizeInBytes) override;
    virtual void* mapPaintAuxBuffer(size_t mapSizeInBytes) override;
    virtual void* mapContourBuffer(size_t mapSizeInBytes) override;
    virtual void* mapGradSpanBuffer(size_t mapSizeInBytes) override;
    virtual void* mapTessVertexSpanBuffer(size_t mapSizeInBytes) override;
    virtual void* mapTriangleVertexBuffer(size_t mapSizeInBytes) override;

    virtual void unmapFlushUniformBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapImageDrawUniformBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapPathBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapPaintBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapPaintAuxBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapContourBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapGradSpanBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapTessVertexSpanBuffer(size_t unmapSizeInBytes) override;
    virtual void unmapTriangleVertexBuffer(size_t unmapSizeInBytes) override;

    virtual rive::rcp<rive::RenderBuffer> makeRenderBuffer(
        rive::RenderBufferType,
        rive::RenderBufferFlags,
        size_t) override;
    virtual void resizeGradientTexture(uint32_t width,
                                       uint32_t height) override;
    virtual void resizeTessellationTexture(uint32_t width,
                                           uint32_t height) override;
    virtual void resizeAtlasTexture(uint32_t width, uint32_t height) override;

    virtual void flush(const rive::gpu::FlushDescriptor&) override;

private:
    DelayLoadedTexture m_gradientTexture;
    DelayLoadedTexture m_tesselationTexture;
    DelayLoadedTexture m_featherAtlasTexture;

    // This is used for unused bindings in Render Graph because it doesn't allow
    // read inputs that aren't written to beforehand.
    FTextureRHIRef m_placeholderTexture;
    // feather texture gets created and uploaded once on construction, so we
    // make it an external texture
    FTextureRHIRef m_featherTexture;

    FBufferRHIRef m_patchVertexBuffer;
    FBufferRHIRef m_patchIndexBuffer;
    FBufferRHIRef m_imageRectVertexBuffer;
    FBufferRHIRef m_imageRectIndexBuffer;
    FBufferRHIRef m_tessSpanIndexBuffer;

    FSamplerStateRHIRef
        m_imageSamplers[rive::ImageSampler::MAX_SAMPLER_PERMUTATIONS];
    FSamplerStateRHIRef m_linearSampler;
    FSamplerStateRHIRef m_atlasSampler;
    FSamplerStateRHIRef m_featherSampler;

    FVertexDeclarationRHIRef VertexDeclarations[static_cast<int32>(
        EVertexDeclarations::NumVertexDeclarations)];

    std::unique_ptr<
        UniformBufferRHIImpl<rive::gpu::FlushUniforms, FFlushUniforms>>
        m_flushUniformBuffer;
    std::unique_ptr<
        UniformBufferRHIImpl<rive::gpu::ImageDrawUniforms, FImageDrawUniforms>>
        m_imageDrawUniformBuffer;
    StructuredBufferRHIImpl<rive::gpu::PathData> m_pathBuffer;
    StructuredBufferRHIImpl<rive::gpu::PaintData> m_paintBuffer;
    StructuredBufferRHIImpl<rive::gpu::PaintAuxData> m_paintAuxBuffer;
    StructuredBufferRHIImpl<rive::gpu::ContourData> m_contourBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_gradSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_tessSpanBuffer;
    std::unique_ptr<BufferRingRHIImpl> m_triangleBuffer;

    std::chrono::steady_clock::time_point m_localEpoch =
        std::chrono::steady_clock::now();

    RHICapabilities m_capabilities;
#if WITH_EDITOR
    rive::gpu::PlatformFeatures m_originalPlatformFeatures;
#endif
};
