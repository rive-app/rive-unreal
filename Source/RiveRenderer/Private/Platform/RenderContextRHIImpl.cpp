#include "RenderContextRHIImpl.hpp"

#include "CommonRenderResources.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "PixelShaderUtils.h"
#include "RenderGraphBuilder.h"

#if UE_VERSION_OLDER_THAN(5, 5, 0)
#include "RHIResourceUpdates.h"
#endif

#include "RenderGraphUtils.h"
#include "Logs/RiveRendererLog.h"

#include "HAL/IConsoleManager.h"

#include "Containers/ResourceArray.h"
#include "RHIStaticStates.h"
#include "Modules/ModuleManager.h"

#include "RenderGraphHelpers/RivePassFunctions.h"

#include "RHICommandList.h"
#include "rive/decoders/bitmap_decoder.hpp"
#include "Misc/EngineVersionComparison.h"

#include "RiveShaderTypes.h"

THIRD_PARTY_INCLUDES_START
#include "rive/renderer/rive_render_image.hpp"
#include "rive/shaders/constants.glsl"
THIRD_PARTY_INCLUDES_END

template <typename DataType, size_t size>
struct TStaticResourceData : public FResourceArrayInterface
{
    DataType Data[size];

public:
    TStaticResourceData() {}

    DataType* operator*() { return Data; }
    /**
     * @return A pointer to the resource data.
     */
    virtual const void* GetResourceData() const override { return Data; }

    /**
     * @return size of resource data allocation (in bytes)
     */
    virtual uint32 GetResourceDataSize() const override
    {
        return size * sizeof(DataType);
    };

    /** Do nothing on discard because this is static const CPU data */
    virtual void Discard() override {}

    virtual bool IsStatic() const override { return true; }

    /**
     * @return true if the resource keeps a copy of its resource data after the
     * RHI resource has been created
     */
    virtual bool GetAllowCPUAccess() const override { return true; }

    /**
     * Sets whether the resource array will be accessed by CPU.
     */
    virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override {}
};

template <typename DataType, size_t size>
struct TStaticExternalResourceData : public FResourceArrayInterface
{
    const DataType (&Data)[size];

public:
    TStaticExternalResourceData(const DataType (&Data)[size]) : Data(Data) {}

    /**
     * @return A pointer to the resource data.
     */
    virtual const void* GetResourceData() const override { return Data; };

    /**
     * @return size of resource data allocation (in bytes)
     */
    virtual uint32 GetResourceDataSize() const override
    {
        return size * sizeof(DataType);
    };

    /** Do nothing on discard because this is static const CPU data */
    virtual void Discard() override {}

    virtual bool IsStatic() const override { return true; }

    /**
     * @return true if the resource keeps a copy of its resource data after the
     * RHI resource has been created
     */
    virtual bool GetAllowCPUAccess() const override { return true; }

    /**
     * Sets whether the resource array will be accessed by CPU.
     */
    virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override {}
};

using namespace rive;
using namespace rive::gpu;

TStaticExternalResourceData GImageRectIndices(kImageRectIndices);
TStaticExternalResourceData GImageRectVertices(kImageRectVertices);
TStaticExternalResourceData GTessSpanIndices(kTessSpanIndices);

TStaticResourceData<PatchVertex, kPatchVertexBufferCount> GPatchVertices;
TStaticResourceData<uint16_t, kPatchIndexBufferCount> GPatchIndices;
// clang format does weird things to this so turn it off
// clang-format off
static TAutoConsoleVariable<int32> CVarShouldVisualizeRive(
    TEXT("r.rive.vis"),
    0,
    TEXT("If non 0, visualize one of many textures / buffers rive uses in "
         "rendering\n") TEXT("<=0: off\n") TEXT("  1: visualize clip\n")
        TEXT("  2: visualize coverage texture\n")
        TEXT("  3: visualize tessalation texture\n")
        TEXT("  4: visuzlize paint data buffer\n"),
    ECVF_Scalability | ECVF_RenderThreadSafe);
// clang-format on

void GetPermutationForFeatures(
    const ShaderFeatures features,
    const ShaderMiscFlags miscFlags,
    const RHICapabilities& Capabilities,
    AtomicPixelPermutationDomain& PixelPermutationDomain,
    AtomicVertexPermutationDomain& VertexPermutationDomain)
{
    VertexPermutationDomain.Set<FEnableClip>(features &
                                             ShaderFeatures::ENABLE_CLIPPING);
    VertexPermutationDomain.Set<FEnableClipRect>(
        features & ShaderFeatures::ENABLE_CLIP_RECT);
    VertexPermutationDomain.Set<FEnableAdvanceBlend>(
        features & ShaderFeatures::ENABLE_ADVANCED_BLEND);
    VertexPermutationDomain.Set<FEnableFeather>(features &
                                                ShaderFeatures::ENABLE_FEATHER);

    PixelPermutationDomain.Set<FEnableClip>(features &
                                            ShaderFeatures::ENABLE_CLIPPING);
    PixelPermutationDomain.Set<FEnableClipRect>(
        features & ShaderFeatures::ENABLE_CLIP_RECT);
    PixelPermutationDomain.Set<FEnableNestedClip>(
        features & ShaderFeatures::ENABLE_NESTED_CLIPPING);
    PixelPermutationDomain.Set<FEnableAdvanceBlend>(
        features & ShaderFeatures::ENABLE_ADVANCED_BLEND);
    PixelPermutationDomain.Set<FEnableFixedFunctionColorOutput>(
        !(features & ShaderFeatures::ENABLE_ADVANCED_BLEND));
    PixelPermutationDomain.Set<FEnableEvenOdd>(features &
                                               ShaderFeatures::ENABLE_EVEN_ODD);
    PixelPermutationDomain.Set<FEnableHSLBlendMode>(
        features & ShaderFeatures::ENABLE_HSL_BLEND_MODES);
    PixelPermutationDomain.Set<FEnableTypedUAVLoads>(
        Capabilities.bSupportsTypedUAVLoads);
    PixelPermutationDomain.Set<FEnableFeather>(features &
                                               ShaderFeatures::ENABLE_FEATHER);
}

/*
 * Convenience function for adding a PF_R32 Texture visualization pass, used for
 * debuging
 */
void AddBltU32ToF4Pass(FRDGBuilder& GraphBuilder,
                       FRDGTextureRef SourceTexture,
                       FRDGTextureRef DestTexture,
                       FIntPoint Size)
{
    // DEBUG STUFF
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    TShaderMapRef<FRiveRDGBltU32AsF4PixelShader> PixelShader(ShaderMap);

    FRiveRDGBltU32AsF4PixelShader::FParameters* BltU32ToF4Parameters =
        GraphBuilder
            .AllocParameters<FRiveRDGBltU32AsF4PixelShader::FParameters>();

    BltU32ToF4Parameters->SourceTexture = SourceTexture;
    BltU32ToF4Parameters->RenderTargets[0] =
        FRenderTargetBinding(DestTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(GraphBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("riv.U32ToF4"),
                                         PixelShader,
                                         BltU32ToF4Parameters,
                                         FIntRect(0, 0, Size.X, Size.Y));
}

/*
 * Convenience function for adding a PF_R32G32B32A32_UINT Texture visualization
 * pass, used for debuging
 */
void AddBltU324ToF4Pass(FRDGBuilder& GraphBuilder,
                        FRDGTextureRef SourceTexture,
                        FRDGTextureRef DestTexture,
                        FIntRect Viewport)
{
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    TShaderMapRef<FRiveRDGBltU324AsF4PixelShader> PixelShader(ShaderMap);

    FRiveRDGBltU324AsF4PixelShader::FParameters* BltU324ToF4Parameters =
        GraphBuilder
            .AllocParameters<FRiveRDGBltU324AsF4PixelShader::FParameters>();

    BltU324ToF4Parameters->SourceTexture = SourceTexture;
    BltU324ToF4Parameters->RenderTargets[0] =
        FRenderTargetBinding(DestTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(GraphBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("riv.U324ToF4"),
                                         PixelShader,
                                         BltU324ToF4Parameters,
                                         Viewport);
}

/*
 * Convenience function for adding a PF_R32G32_UINT Buffer visualization pass,
 * used for debuging
 */
void AddVisualizeBufferPass(FRDGBuilder& GraphBuilder,
                            int32 BufferSize,
                            FRDGBufferSRVRef SourceBuffer,
                            FRDGTextureRef DestTexture,
                            FIntPoint Size)
{
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    TShaderMapRef<FRiveRDGVisualizeBufferPixelShader> PixelShader(ShaderMap);
    FRiveRDGVisualizeBufferPixelShader::FParameters* VisBufferParameters =
        GraphBuilder
            .AllocParameters<FRiveRDGVisualizeBufferPixelShader::FParameters>();
    VisBufferParameters->ViewSize = FUintVector2(Size.X, Size.Y);
    VisBufferParameters->BufferSize = BufferSize;

    VisBufferParameters->SourceBuffer = SourceBuffer;
    VisBufferParameters->RenderTargets[0] =
        FRenderTargetBinding(DestTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(GraphBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("riv.VisBuffer"),
                                         PixelShader,
                                         VisBufferParameters,
                                         FIntRect(0, 0, Size.X, Size.Y));
}

RHICapabilities::RHICapabilities()
{
    bSupportsPixelShaderUAVs = GRHISupportsPixelShaderUAVs;
    // right now this is required to run rhi.
    check(bSupportsPixelShaderUAVs);
    check(UE::PixelFormat::HasCapabilities(PF_R8G8B8A8,
                                           EPixelFormatCapabilities::UAV));
    check(UE::PixelFormat::HasCapabilities(PF_B8G8R8A8,
                                           EPixelFormatCapabilities::UAV));
#if UE_VERSION_OLDER_THAN(5, 4, 0)
    // for now just force metal as the only raster order support until there is
    // a better version in 5.3
    bSupportsRasterOrderViews =
        GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal;
#else
    bSupportsRasterOrderViews = GRHISupportsRasterOrderViews;
#endif
    // it seems vulkan requires typed uav support.
    bSupportsTypedUAVLoads =
        RHISupports4ComponentUAVReadWrite(GMaxRHIShaderPlatform) ||
        GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan;
}

template <typename DataType>
FBufferRHIRef makeSimpleImmutableBuffer(FRHICommandList& RHICmdList,
                                        const TCHAR* DebugName,
                                        EBufferUsageFlags bindFlags,
                                        FResourceArrayInterface& ResourceArray)
{
    const size_t size = ResourceArray.GetResourceDataSize();
    FRHIResourceCreateInfo Info(DebugName, &ResourceArray);
    auto buffer = RHICmdList.CreateBuffer(size,
                                          EBufferUsageFlags::Static | bindFlags,
                                          sizeof(DataType),
                                          ERHIAccess::VertexOrIndexBuffer,
                                          Info);
    return buffer;
}

BufferRingRHIImpl::BufferRingRHIImpl(EBufferUsageFlags flags,
                                     size_t inSizeInBytes,
                                     size_t stride) :
    BufferRing(inSizeInBytes), m_flags(flags), m_stride(stride)
{}

FBufferRHIRef BufferRingRHIImpl::Sync(FRHICommandList& commandList,
                                      size_t offsetInBytes) const
{
    FRHIResourceCreateInfo Info(TEXT("rive.BufferRingRHIImpl_"));

    const size_t size = capacityInBytes() - offsetInBytes;
    auto buffer =
        commandList.CreateBuffer(size,
                                 m_flags | EBufferUsageFlags::Volatile,
                                 m_stride,
                                 ERHIAccess::WriteOnlyMask,
                                 Info);
    // for DX12 we should use RLM_WriteOnly_NoOverwrite but RLM_WriteOnly works
    // everywhere so we use it for now
    auto map = commandList.LockBuffer(buffer, 0, size, RLM_WriteOnly);
    memcpy(map, shadowBuffer() + offsetInBytes, size);
    commandList.UnlockBuffer(buffer);

    return buffer;
}

FRDGBufferRef BufferRingRHIImpl::Sync(FRDGBuilder& RDGBuilder,
                                      size_t offsetInBytes) const
{
    const size_t size = capacityInBytes() - offsetInBytes;
    // clang was trying to do a copy constructor here for some crazy reason on
    // mac. This prevents that
    FRDGBufferDesc Desc{static_cast<uint32>(m_stride),
                        static_cast<uint32>(size / m_stride),
                        m_flags | EBufferUsageFlags::Volatile};
    auto buffer = RDGBuilder.CreateBuffer(Desc,
                                          TEXT("rive.BufferRingRHIImpl_"),
                                          ERDGBufferFlags::None);
    RDGBuilder.QueueBufferUpload(buffer,
                                 shadowBuffer() + offsetInBytes,
                                 size,
                                 ERDGInitialDataFlags::NoCopy);
    return buffer;
}

void* BufferRingRHIImpl::onMapBuffer(int bufferIdx, size_t mapSizeInBytes)
{
    return shadowBuffer();
}

void BufferRingRHIImpl::onUnmapAndSubmitBuffer(int bufferIdx,
                                               size_t mapSizeInBytes)
{}

RenderBufferRHIImpl::RenderBufferRHIImpl(RenderBufferType inType,
                                         RenderBufferFlags inFlags,
                                         size_t inSizeInBytes,
                                         size_t stride) :
    lite_rtti_override(inType, inFlags, inSizeInBytes),
    m_buffer(inType == RenderBufferType::vertex
                 ? EBufferUsageFlags::VertexBuffer
                 : EBufferUsageFlags::IndexBuffer,
             inSizeInBytes,
             stride),
    m_mappedBuffer(nullptr)
{
    if (inFlags & RenderBufferFlags::mappedOnceAtInitialization)
    {
        m_mappedBuffer = m_buffer.mapBuffer(inSizeInBytes);
    }
}

FBufferRHIRef RenderBufferRHIImpl::Sync(FRHICommandList& commandList) const
{
    return m_buffer.Sync(commandList);
}

void* RenderBufferRHIImpl::onMap()
{
    if (flags() & RenderBufferFlags::mappedOnceAtInitialization)
    {
        check(m_mappedBuffer);
        return m_mappedBuffer;
    }
    return m_buffer.mapBuffer(sizeInBytes());
}

void RenderBufferRHIImpl::onUnmap() { m_buffer.unmapAndSubmitBuffer(); }

#if UE_VERSION_OLDER_THAN(5, 5, 0)
class TextureRHIImpl : public Texture
{
public:
    TextureRHIImpl(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevelCount,
                   const uint8_t* imageData,
                   EPixelFormat PixelFormat = PF_B8G8R8A8) :
        Texture(width, height)
    {
        FRHIAsyncCommandList commandList;
        FRHICommandListScopedPipelineGuard Guard(*commandList);

        auto Desc =
            FRHITextureCreateDesc::Create2D(TEXT("rive.PLSTextureRHIImpl_"),
                                            m_width,
                                            m_height,
                                            PixelFormat);
        Desc.SetNumMips(mipLevelCount);
        m_texture = CREATE_TEXTURE_ASYNC(commandList, Desc);
        commandList->UpdateTexture2D(
            m_texture,
            0,
            FUpdateTextureRegion2D(0, 0, 0, 0, m_width, m_height),
            m_width * 4,
            imageData);
    }

    FRDGTextureRef asRDGTexture(FRDGBuilder& Builder) const
    {
        check(m_texture);
        return Builder.RegisterExternalTexture(
            CreateRenderTarget(m_texture, TEXT("rive.PLSTextureRHIImpl_")));
    }

    virtual ~TextureRHIImpl() override {}

    FTextureRHIRef contents() const { return m_texture; }

private:
    FTextureRHIRef m_texture;
};
#else // UE VERSION > 5_5:
// FRHIAsyncCommandList was removed in 5.5 we should probably defer load these
// instead but this works for now
class TextureRHIImpl : public Texture
{
public:
    TextureRHIImpl(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevelCount,
                   const uint8_t* imageData,
                   EPixelFormat PixelFormat = PF_B8G8R8A8) :
        Texture(width, height)
    {
        FRHICommandList& commandList =
            GRHICommandList.GetImmediateCommandList();
        FRHICommandListScopedPipelineGuard Guard(commandList);

        auto Desc =
            FRHITextureCreateDesc::Create2D(TEXT("rive.PLSTextureRHIImpl_"),
                                            m_width,
                                            m_height,
                                            PixelFormat)
                .AddFlags(ETextureCreateFlags::SRGB);
        Desc.SetNumMips(mipLevelCount);
        m_texture = CREATE_TEXTURE_ASYNC(commandList, Desc);
        commandList.UpdateTexture2D(
            m_texture,
            0,
            FUpdateTextureRegion2D(0, 0, 0, 0, m_width, m_height),
            m_width * 4,
            imageData);
    }

    FRDGTextureRef asRDGTexture(FRDGBuilder& Builder) const
    {
        check(m_texture);
        return Builder.RegisterExternalTexture(
            CreateRenderTarget(m_texture, TEXT("rive.PLSTextureRHIImpl_")));
    }

    virtual ~TextureRHIImpl() override {}

    FTextureRHIRef contents() const { return m_texture; }

private:
    FTextureRHIRef m_texture;
};
#endif

FString RHICapabilities::AsString() const
{
    return FString::Printf(TEXT("{ bSupportsPixelShaderUAVs %i,"
                                " bSupportsTypedUAVLoads %i, "
                                "bSupportsRasterOrderViews %i}"),
                           bSupportsPixelShaderUAVs,
                           bSupportsTypedUAVLoads,
                           bSupportsRasterOrderViews);
}

RenderTargetRHI::RenderTargetRHI(FRHICommandList& RHICmdList,
                                 const RHICapabilities& Capabilities,
                                 const FTextureRHIRef& InTextureTarget) :
    RenderTarget(InTextureTarget->GetSizeX(), InTextureTarget->GetSizeY()),
    m_textureTarget(InTextureTarget),
    m_capabilities(Capabilities)
{
    FRHITextureCreateDesc coverageDesc =
        FRHITextureCreateDesc::Create2D(TEXT("rive.AtomicCoverage"),
                                        width(),
                                        height(),
                                        PF_R32_UINT);
    coverageDesc.SetNumMips(1);
    coverageDesc.AddFlags(ETextureCreateFlags::UAV);
    m_atomicCoverageTexture = CREATE_TEXTURE(RHICmdList, coverageDesc);

    FRHITextureCreateDesc clipDesc =
        FRHITextureCreateDesc::Create2D(TEXT("rive.Clip"),
                                        width(),
                                        height(),
                                        PF_R32_UINT);
    clipDesc.SetNumMips(1);
    clipDesc.AddFlags(ETextureCreateFlags::UAV);
    m_clipTexture = CREATE_TEXTURE(RHICmdList, clipDesc);

    m_targetTextureSupportsUAV = static_cast<bool>(
        m_textureTarget->GetDesc().Flags & ETextureCreateFlags::UAV);

    // Rive is not currently supported without UAVs.
    check(Capabilities.bSupportsPixelShaderUAVs);
}

FRDGTextureRef RenderTargetRHI::targetTexture(FRDGBuilder& Builder)
{
    return Builder.RegisterExternalTexture(
        CreateRenderTarget(m_textureTarget, TEXT("rive.TargetTexture")),
        ERDGTextureFlags::MultiFrame);
}

FRDGTextureRef RenderTargetRHI::clipTexture(FRDGBuilder& Builder)
{
    return Builder.RegisterExternalTexture(
        CreateRenderTarget(m_clipTexture, TEXT("rive.Clip")),
        ERDGTextureFlags::MultiFrame);
}

FRDGTextureRef RenderTargetRHI::coverageTexture(FRDGBuilder& Builder)
{
    return Builder.RegisterExternalTexture(
        CreateRenderTarget(m_atomicCoverageTexture,
                           TEXT("rive.AtomicCoverage")),
        ERDGTextureFlags::MultiFrame);
}

void DelayLoadedTexture::UpdateTexture(const FRDGTextureDesc& inDesc,
                                       FString DebugName,
                                       bool inNeedsSRV)
{
    m_rdgDesc = inDesc;
    m_debugName = DebugName;
}

void DelayLoadedTexture::Sync(FRDGBuilder& RDGBuilder,
                              FRDGTextureRef* outTexture,
                              FRDGTextureSRVRef* outSRV) const
{
    check(outTexture);

    *outTexture = RDGBuilder.CreateTexture(m_rdgDesc, *m_debugName);

    if (outSRV)
    {
        FRDGTextureSRVDesc Desc = FRDGTextureSRVDesc::Create(*outTexture);
        *outSRV = RDGBuilder.CreateSRV(Desc);
    }
}

std::unique_ptr<RenderContext> RenderContextRHIImpl::MakeContext(
    FRHICommandListImmediate& CommandListImmediate)
{
    auto plsContextImpl =
        std::make_unique<RenderContextRHIImpl>(CommandListImmediate);
    return std::make_unique<RenderContext>(std::move(plsContextImpl));
}

RenderContextRHIImpl::RenderContextRHIImpl(
    FRHICommandListImmediate& CommandListImmediate)
{
    auto CapabilityString = m_capabilities.AsString();
    UE_LOG(LogRiveRenderer,
           Display,
           TEXT("Running RHI with %s Capabilities"),
           *CapabilityString);

    m_platformFeatures.supportsFragmentShaderAtomics =
        m_capabilities.bSupportsPixelShaderUAVs;
    m_platformFeatures.supportsClipPlanes = true;
    m_platformFeatures.supportsRasterOrdering =
        false; // m_capabilities.bSupportsRasterOrderViews;
    m_platformFeatures.clipSpaceBottomUp = true;
    m_platformFeatures.framebufferBottomUp = false;

    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    VertexDeclarations[static_cast<int32>(EVertexDeclarations::Resolve)] =
        GEmptyVertexDeclaration.VertexDeclarationRHI;

    FVertexDeclarationElementList pathElementList;
    pathElementList.Add(FVertexElement(
        FVertexElement(0, 0, VET_Float4, 0, sizeof(PatchVertex), false)));
    pathElementList.Add(FVertexElement(FVertexElement(0,
                                                      sizeof(float4),
                                                      VET_Float4,
                                                      1,
                                                      sizeof(PatchVertex),
                                                      false)));
    VertexDeclarations[static_cast<int32>(EVertexDeclarations::Paths)] =
        PipelineStateCache::GetOrCreateVertexDeclaration(pathElementList);

    FVertexDeclarationElementList trianglesElementList;
    trianglesElementList.Add(
        FVertexElement(0, 0, VET_Float3, 0, sizeof(TriangleVertex), false));
    VertexDeclarations[static_cast<int32>(
        EVertexDeclarations::InteriorTriangles)] =
        PipelineStateCache::GetOrCreateVertexDeclaration(trianglesElementList);

    FVertexDeclarationElementList ImageMeshElementList;
    ImageMeshElementList.Add(
        FVertexElement(0, 0, VET_Float2, 0, sizeof(Vec2D), false));
    ImageMeshElementList.Add(
        FVertexElement(1, 0, VET_Float2, 1, sizeof(Vec2D), false));
    VertexDeclarations[static_cast<int32>(EVertexDeclarations::ImageMesh)] =
        PipelineStateCache::GetOrCreateVertexDeclaration(ImageMeshElementList);

    FVertexDeclarationElementList SpanElementList;
    SpanElementList.Add(
        FVertexElement(0, 0, VET_UInt, 0, sizeof(GradientSpan), true));
    SpanElementList.Add(
        FVertexElement(0, 4, VET_UInt, 1, sizeof(GradientSpan), true));
    SpanElementList.Add(
        FVertexElement(0, 8, VET_UInt, 2, sizeof(GradientSpan), true));
    SpanElementList.Add(
        FVertexElement(0, 12, VET_UInt, 3, sizeof(GradientSpan), true));
    VertexDeclarations[static_cast<int32>(EVertexDeclarations::Gradient)] =
        PipelineStateCache::GetOrCreateVertexDeclaration(SpanElementList);

    FVertexDeclarationElementList TessElementList;
    size_t tessOffset = 0;
    size_t tessStride = sizeof(TessVertexSpan);
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_Float4, 0, tessStride, true));
    tessOffset += 4 * sizeof(float);
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_Float4, 1, tessStride, true));
    tessOffset += 4 * sizeof(float);
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_Float4, 2, tessStride, true));
    tessOffset += 4 * sizeof(float);
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_UInt, 3, tessStride, true));
    tessOffset += 4;
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_UInt, 4, tessStride, true));
    tessOffset += 4;
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_UInt, 5, tessStride, true));
    tessOffset += 4;
    TessElementList.Add(
        FVertexElement(0, tessOffset, VET_UInt, 6, tessStride, true));
    check(tessOffset + 4 == sizeof(TessVertexSpan));

    VertexDeclarations[static_cast<int32>(EVertexDeclarations::Tessellation)] =
        PipelineStateCache::GetOrCreateVertexDeclaration(TessElementList);

    FVertexDeclarationElementList ImageRectVertexElementList;
    ImageRectVertexElementList.Add(
        FVertexElement(0, 0, VET_Float4, 0, sizeof(ImageRectVertex), false));
    auto ImageRectDecleration =
        PipelineStateCache::GetOrCreateVertexDeclaration(
            ImageRectVertexElementList);
    VertexDeclarations[static_cast<int32>(EVertexDeclarations::ImageRect)] =
        ImageRectDecleration;

    FVertexDeclarationElementList AtlasVertexElementList;
    AtlasVertexElementList.Add(FVertexElement(
        FVertexElement(0, 0, VET_Float4, 0, sizeof(PatchVertex), false)));
    AtlasVertexElementList.Add(
        FVertexElement(FVertexElement(0,
                                      sizeof(float4),
                                      VET_Float4,
                                      1,
                                      sizeof(PatchVertex),
                                      false)));
    auto AtlasDecleration = PipelineStateCache::GetOrCreateVertexDeclaration(
        AtlasVertexElementList);
    VertexDeclarations[static_cast<int32>(EVertexDeclarations::Atlas)] =
        AtlasDecleration;

    GeneratePatchBufferData(*GPatchVertices, *GPatchIndices);

    m_patchVertexBuffer =
        makeSimpleImmutableBuffer<PatchVertex>(CommandListImmediate,
                                               TEXT("rive.PatchVertexBuffer"),
                                               EBufferUsageFlags::VertexBuffer,
                                               GPatchVertices);
    m_patchIndexBuffer =
        makeSimpleImmutableBuffer<uint16_t>(CommandListImmediate,
                                            TEXT("rive.PatchIndexBuffer"),
                                            EBufferUsageFlags::IndexBuffer,
                                            GPatchIndices);

    m_tessSpanIndexBuffer =
        makeSimpleImmutableBuffer<uint16_t>(CommandListImmediate,
                                            TEXT("rive.TessIndexBuffer"),
                                            EBufferUsageFlags::IndexBuffer,
                                            GTessSpanIndices);

    m_imageRectVertexBuffer = makeSimpleImmutableBuffer<ImageRectVertex>(
        CommandListImmediate,
        TEXT("rive.ImageRectVertexBuffer"),
        EBufferUsageFlags::VertexBuffer,
        GImageRectVertices);

    m_imageRectIndexBuffer =
        makeSimpleImmutableBuffer<uint16>(CommandListImmediate,
                                          TEXT("rive.ImageRectIndexBuffer"),
                                          EBufferUsageFlags::IndexBuffer,
                                          GImageRectIndices);

    m_atlasSampler = TStaticSamplerState<SF_Point,
                                         AM_Clamp,
                                         AM_Clamp,
                                         AM_Clamp,
                                         0,
                                         1,
                                         0,
                                         SCF_Never>::GetRHI();
    m_mipmapSampler = TStaticSamplerState<SF_Point,
                                          AM_Clamp,
                                          AM_Clamp,
                                          AM_Clamp,
                                          0,
                                          1,
                                          0,
                                          SCF_Never>::GetRHI();
    m_featherSampler = TStaticSamplerState<SF_Bilinear,
                                           AM_Clamp,
                                           AM_Clamp,
                                           AM_Clamp,
                                           0,
                                           1,
                                           0,
                                           SCF_Never>::GetRHI();
    m_linearSampler = TStaticSamplerState<SF_Bilinear,
                                          AM_Clamp,
                                          AM_Clamp,
                                          AM_Clamp,
                                          0,
                                          1,
                                          0,
                                          SCF_Never>::GetRHI();

    auto PlaceholderDesc =
        FRHITextureCreateDesc::Create2D(TEXT("rive.Placeholder"),
                                        FIntPoint(1, 1),
                                        PF_R32_UINT);
    PlaceholderDesc.AddFlags(ETextureCreateFlags::ShaderResource);
    m_placeholderTexture =
        CREATE_TEXTURE(CommandListImmediate, PlaceholderDesc);

    auto featherTextureDesc =
        FRHITextureCreateDesc::Create2DArray(TEXT("rive.FeatherTexture"),
                                             {gpu::GAUSSIAN_TABLE_SIZE, 1},
                                             FEATHER_TEXTURE_1D_ARRAY_LENGTH,
                                             PF_R16F)
            .AddFlags(ETextureCreateFlags::ShaderResource);
    m_featherTexture = CREATE_TEXTURE(CommandListImmediate, featherTextureDesc);
    // update the texture to contain feather data
    uint32 stride;
    void* data = CommandListImmediate.LockTexture2DArray(
        m_featherTexture,
        FEATHER_FUNCTION_ARRAY_INDEX,
        0,
        EResourceLockMode::RLM_WriteOnly,
        stride,
        false);
    memcpy(data,
           gpu::g_gaussianIntegralTableF16,
           sizeof(gpu::g_gaussianIntegralTableF16));
    CommandListImmediate.UnlockTexture2DArray(m_featherTexture,
                                              FEATHER_FUNCTION_ARRAY_INDEX,
                                              0,
                                              false);

    data = CommandListImmediate.LockTexture2DArray(
        m_featherTexture,
        FEATHER_INVERSE_FUNCTION_ARRAY_INDEX,
        0,
        EResourceLockMode::RLM_WriteOnly,
        stride,
        false);
    memcpy(data,
           gpu::g_inverseGaussianIntegralTableF16,
           sizeof(gpu::g_inverseGaussianIntegralTableF16));
    CommandListImmediate.UnlockTexture2DArray(
        m_featherTexture,
        FEATHER_INVERSE_FUNCTION_ARRAY_INDEX,
        0,
        false);
}

rcp<RenderTargetRHI> RenderContextRHIImpl::makeRenderTarget(
    FRHICommandListImmediate& RHICmdList,
    const FTextureRHIRef& InTargetTexture)
{
    return make_rcp<RenderTargetRHI>(RHICmdList,
                                     m_capabilities,
                                     InTargetTexture);
}

rcp<Texture> RenderContextRHIImpl::decodeImageTexture(
    Span<const uint8_t> encodedBytes)
{
    constexpr uint8_t PNG_FINGERPRINT[4] = {0x89, 0x50, 0x4E, 0x47};
    constexpr uint8_t JPEG_FINGERPRINT[3] = {0xFF, 0xD8, 0xFF};
    constexpr uint8_t WEBP_FINGERPRINT[3] = {0x52, 0x49, 0x46};

    EImageFormat format = EImageFormat::Invalid;

    // we do not have enough size to be anything
    if (encodedBytes.size() < sizeof(PNG_FINGERPRINT))
    {
        return nullptr;
    }

    if (memcmp(PNG_FINGERPRINT, encodedBytes.data(), sizeof(PNG_FINGERPRINT)) ==
        0)
    {
        format = EImageFormat::PNG;
    }
    else if (memcmp(JPEG_FINGERPRINT,
                    encodedBytes.data(),
                    sizeof(JPEG_FINGERPRINT)) == 0)
    {
        format = EImageFormat::JPEG;
    }
    else if (memcmp(WEBP_FINGERPRINT,
                    encodedBytes.data(),
                    sizeof(WEBP_FINGERPRINT)) == 0)
    {
        format = EImageFormat::Invalid;
    }
    else
    {
        RIVE_DEBUG_ERROR("Invalid Decode Image header");
        return nullptr;
    }

    std::unique_ptr<Bitmap> bitmap;
    if (format != EImageFormat::Invalid)
    {
        // Use Unreal for PNG and JPEG
        IImageWrapperModule& ImageWrapperModule =
            FModuleManager::LoadModuleChecked<IImageWrapperModule>(
                FName("ImageWrapper"));
        TSharedPtr<IImageWrapper> ImageWrapper =
            ImageWrapperModule.CreateImageWrapper(format);
        if (!ImageWrapper.IsValid() ||
            !ImageWrapper->SetCompressed(encodedBytes.data(),
                                         encodedBytes.size()))
        {
            return nullptr;
        }

        TArray<uint8> UncompressedRGBA;
        if (!ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, UncompressedRGBA))
        {
            return nullptr;
        }

        uint8* data = new uint8[UncompressedRGBA.Num()];
        memcpy(data,
               UncompressedRGBA.GetData(),
               UncompressedRGBA.Num() * sizeof(uint8));

        bitmap = std::make_unique<Bitmap>(ImageWrapper->GetWidth(),
                                          ImageWrapper->GetHeight(),
                                          Bitmap::PixelFormat::RGBA,
                                          data);
    }
    else
    {
        // WEBP Decoding, using the built in rive method
        bitmap = Bitmap::decode(encodedBytes.data(), encodedBytes.size());
        if (!bitmap)
        {
            RIVE_DEBUG_ERROR("Webp Decoding Failed !");
            return nullptr;
        }
    }

    if (bitmap->pixelFormat() != Bitmap::PixelFormat::RGBAPremul)
    {
        bitmap->pixelFormat(Bitmap::PixelFormat::RGBAPremul);
    }
    return make_rcp<TextureRHIImpl>(bitmap->width(),
                                    bitmap->height(),
                                    1,
                                    bitmap->bytes(),
                                    EPixelFormat::PF_R8G8B8A8);
}

void RenderContextRHIImpl::resizeFlushUniformBuffer(size_t sizeInBytes)
{
    m_flushUniformBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_flushUniformBuffer = std::make_unique<
            UniformBufferRHIImpl<FlushUniforms, FFlushUniforms>>(sizeInBytes);
    }
}

void RenderContextRHIImpl::resizeImageDrawUniformBuffer(size_t sizeInBytes)
{
    m_imageDrawUniformBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_imageDrawUniformBuffer = std::make_unique<
            UniformBufferRHIImpl<ImageDrawUniforms, FImageDrawUniforms>>(
            sizeInBytes);
    }
}

void RenderContextRHIImpl::resizePathBuffer(size_t sizeInBytes,
                                            StorageBufferStructure structure)
{
    m_pathBuffer.Resize(sizeInBytes,
                        StorageBufferElementSizeInBytes(structure));
}

void RenderContextRHIImpl::resizePaintBuffer(size_t sizeInBytes,
                                             StorageBufferStructure structure)
{
    m_paintBuffer.Resize(sizeInBytes,
                         StorageBufferElementSizeInBytes(structure));
}

void RenderContextRHIImpl::resizePaintAuxBuffer(
    size_t sizeInBytes,
    StorageBufferStructure structure)
{
    m_paintAuxBuffer.Resize(sizeInBytes,
                            StorageBufferElementSizeInBytes(structure));
}

void RenderContextRHIImpl::resizeContourBuffer(size_t sizeInBytes,
                                               StorageBufferStructure structure)
{
    m_contourBuffer.Resize(sizeInBytes,
                           StorageBufferElementSizeInBytes(structure));
}

void RenderContextRHIImpl::resizeGradSpanBuffer(size_t sizeInBytes)
{
    m_gradSpanBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_gradSpanBuffer =
            std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer,
                                                sizeInBytes,
                                                sizeof(GradientSpan));
    }
}

void RenderContextRHIImpl::resizeTessVertexSpanBuffer(size_t sizeInBytes)
{
    m_tessSpanBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_tessSpanBuffer =
            std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer,
                                                sizeInBytes,
                                                sizeof(TessVertexSpan));
    }
}

void RenderContextRHIImpl::resizeTriangleVertexBuffer(size_t sizeInBytes)
{
    m_triangleBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_triangleBuffer =
            std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer,
                                                sizeInBytes,
                                                sizeof(TriangleVertex));
    }
}

void* RenderContextRHIImpl::mapFlushUniformBuffer(size_t mapSizeInBytes)
{
    return m_flushUniformBuffer->mapBuffer(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapImageDrawUniformBuffer(size_t mapSizeInBytes)
{
    return m_imageDrawUniformBuffer->mapBuffer(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapPathBuffer(size_t mapSizeInBytes)
{
    return m_pathBuffer.Map(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapPaintBuffer(size_t mapSizeInBytes)
{
    return m_paintBuffer.Map(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapPaintAuxBuffer(size_t mapSizeInBytes)
{
    return m_paintAuxBuffer.Map(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapContourBuffer(size_t mapSizeInBytes)
{
    return m_contourBuffer.Map(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapGradSpanBuffer(size_t mapSizeInBytes)
{
    return m_gradSpanBuffer->mapBuffer(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapTessVertexSpanBuffer(size_t mapSizeInBytes)
{
    return m_tessSpanBuffer->mapBuffer(mapSizeInBytes);
}

void* RenderContextRHIImpl::mapTriangleVertexBuffer(size_t mapSizeInBytes)
{
    return m_triangleBuffer->mapBuffer(mapSizeInBytes);
}

void RenderContextRHIImpl::unmapFlushUniformBuffer() {}

void RenderContextRHIImpl::unmapImageDrawUniformBuffer() {}

void RenderContextRHIImpl::unmapPathBuffer() {}

void RenderContextRHIImpl::unmapPaintBuffer() {}

void RenderContextRHIImpl::unmapPaintAuxBuffer() {}

void RenderContextRHIImpl::unmapContourBuffer() {}

void RenderContextRHIImpl::unmapGradSpanBuffer()
{
    m_gradSpanBuffer->unmapAndSubmitBuffer();
}

void RenderContextRHIImpl::unmapTessVertexSpanBuffer()
{
    m_tessSpanBuffer->unmapAndSubmitBuffer();
}

void RenderContextRHIImpl::unmapTriangleVertexBuffer()
{
    m_triangleBuffer->unmapAndSubmitBuffer();
}

rcp<RenderBuffer> RenderContextRHIImpl::makeRenderBuffer(
    RenderBufferType type,
    RenderBufferFlags flags,
    size_t sizeInBytes)
{
    if (sizeInBytes == 0)
        return nullptr;

    return make_rcp<RenderBufferRHIImpl>(
        type,
        flags,
        sizeInBytes,
        type == RenderBufferType::index ? sizeof(uint16_t) : 0);
}

void RenderContextRHIImpl::resizeGradientTexture(uint32_t width,
                                                 uint32_t height)
{
    check(IsInRenderingThread());

    height = std::max(1u, height);

    auto RDGDesc = FRDGTextureDesc::Create2D(
        {static_cast<int32_t>(width), static_cast<int32_t>(height)},
        PF_R8G8B8A8,
        FClearValueBinding(FLinearColor::Red),
        ETextureCreateFlags::RenderTargetable |
            ETextureCreateFlags::ShaderResource);

    m_gradientTexture.UpdateTexture(RDGDesc, TEXT("rive.GradientTexture"));
}

void RenderContextRHIImpl::resizeTessellationTexture(uint32_t width,
                                                     uint32_t height)
{
    check(IsInRenderingThread());

    height = std::max(1u, height);

    auto RDGDesc = FRDGTextureDesc::Create2D(
        {static_cast<int32_t>(width), static_cast<int32_t>(height)},
        PF_R32G32B32A32_UINT,
        FClearValueBinding::Black,
        ETextureCreateFlags::RenderTargetable |
            ETextureCreateFlags::ShaderResource);

    m_tesselationTexture.UpdateTexture(RDGDesc, TEXT("rive.TessTexture"), true);
}

void RenderContextRHIImpl::resizeAtlasTexture(uint32_t width, uint32_t height)
{
    check(IsInRenderingThread());

    width = std::max(1u, width);
    height = std::max(1u, height);

    // TODO: Check for rhi support for r32 render target, some android do not
    // support this
    auto RDGDesc = FRDGTextureDesc::Create2D(
        {static_cast<int32_t>(width), static_cast<int32_t>(height)},
        PF_R32_FLOAT,
        FClearValueBinding::Black,
        ETextureCreateFlags::RenderTargetable |
            ETextureCreateFlags::ShaderResource);

    m_featherAtlasTexture.UpdateTexture(RDGDesc,
                                        TEXT("rive.FeatherAtlasTexture"),
                                        true);
}

DECLARE_GPU_STAT_NAMED(STAT_RiveFlush, TEXT("Rive Flush"));
DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_RiveBufferSync, TEXT("Rive Buffer Sync"));
DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_RiveClearCoverageClip,
                       TEXT("Rive Clear Coverage and Clip"));
DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_SimpleGradientCopy,
                       TEXT("Rive Simple Gradient Copy"));

DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_RiveComplexGradient,
                       TEXT("Rive Complex Gradient Draw"));
DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_RiveTess, TEXT("Rive Tess Draw"));
DECLARE_GPU_STAT_NAMED(STAT_RiveFlush_RiveFlushRenderPass,
                       TEXT("Rive Flush Render Pass"));

void RenderContextRHIImpl::flush(const FlushDescriptor& desc)
{
    check(IsInRenderingThread());

    auto renderTarget = static_cast<RenderTargetRHI*>(desc.renderTarget);
    check(renderTarget);

    FRHICommandList& CommandList = GRHICommandList.GetImmediateCommandList();
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    FRDGBuilder GraphBuilder(CommandList.GetAsImmediate());
    {
        RDG_GPU_STAT_SCOPE(GraphBuilder, STAT_RiveFlush);

        auto targetTexture = renderTarget->targetTexture(GraphBuilder);
        check(targetTexture);
        auto clipTexture = renderTarget->clipTexture(GraphBuilder);
        check(clipTexture);
        auto coverageTexture = renderTarget->coverageTexture(GraphBuilder);
        check(coverageTexture);

        check(renderTarget->TargetTextureSupportsUAV());
        FRDGTextureUAVRef targetUAV = nullptr;

        if (m_capabilities.bSupportsTypedUAVLoads)
        {
            targetUAV = GraphBuilder.CreateUAV(targetTexture);
        }
        else
        {
            targetUAV =
                GraphBuilder.CreateUAV(targetTexture,
                                       ERDGUnorderedAccessViewFlags::None,
                                       PF_R32_UINT);
        }

        auto clipUAV =
            GraphBuilder.CreateUAV(clipTexture,
                                   ERDGUnorderedAccessViewFlags::None,
                                   PF_R32_UINT);

        auto coverageUAV =
            GraphBuilder.CreateUAV(coverageTexture,
                                   ERDGUnorderedAccessViewFlags::None,
                                   PF_R32_UINT);

        check(m_flushUniformBuffer);
        auto flushUniforms =
            m_flushUniformBuffer->Sync(GraphBuilder,
                                       desc.flushUniformDataOffsetInBytes);

        auto placeholderTexture = GraphBuilder.RegisterExternalTexture(
            CreateRenderTarget(m_placeholderTexture, TEXT("RivePlaceholder")));
        auto placeholderSRV = GraphBuilder.CreateSRV(placeholderTexture);

        FRDGTextureRef gradiantTexture = placeholderTexture;
        FRDGTextureRef tesselationTexture = placeholderTexture;
        FRDGTextureSRVRef tessSRV = placeholderSRV;

        FRDGTextureRef atlasTexture = placeholderTexture;

        FRDGTextureRef featherTexture = GraphBuilder.RegisterExternalTexture(
            CreateRenderTarget(m_featherTexture, TEXT("Rive.FeatherTexture")));

        FRDGBufferSRVRef pathSRV = nullptr;
        FRDGBufferSRVRef paintSRV = nullptr;
        FRDGBufferSRVRef paintAuxSRV = nullptr;
        FRDGBufferSRVRef contourSRV = nullptr;

        if (desc.pathCount > 0)
        {
            pathSRV = m_pathBuffer.SyncSRV(GraphBuilder,
                                           desc.firstPath,
                                           desc.pathCount);
            paintSRV = m_paintBuffer.SyncSRV(GraphBuilder,
                                             desc.firstPaint,
                                             desc.pathCount);
            paintAuxSRV = m_paintAuxBuffer.SyncSRV(GraphBuilder,
                                                   desc.firstPaintAux,
                                                   desc.pathCount);
        }

        if (desc.contourCount > 0)
        {
            contourSRV = m_contourBuffer.SyncSRV(GraphBuilder,
                                                 desc.firstContour,
                                                 desc.contourCount);
        }

        FBufferRHIRef triangleBuffer = nullptr;
        // TODO: only sync on first flush of frame
        if (m_triangleBuffer)
        {
            triangleBuffer = m_triangleBuffer->Sync(CommandList);
        }

        {
            RDG_GPU_STAT_SCOPE(GraphBuilder,
                               STAT_RiveFlush_RiveClearCoverageClip);
            check(coverageUAV);
            AddClearUAVPass(GraphBuilder,
                            coverageUAV,
                            FUintVector4(desc.coverageClearValue,
                                         desc.coverageClearValue,
                                         desc.coverageClearValue,
                                         desc.coverageClearValue));

            if (desc.combinedShaderFeatures &
                gpu::ShaderFeatures::ENABLE_CLIPPING)
            {
                AddClearUAVPass(GraphBuilder, clipUAV, FUintVector4(0));
            }
        }

        if (desc.gradSpanCount > 0)
        {
            RDG_GPU_STAT_SCOPE(GraphBuilder,
                               STAT_RiveFlush_RiveComplexGradient);
            m_gradientTexture.Sync(GraphBuilder, &gradiantTexture);

            check(gradiantTexture);
            check(m_gradSpanBuffer);
            auto gradSpanBuffer = m_gradSpanBuffer->Sync(
                CommandList,
                desc.firstGradSpan * sizeof(GradientSpan));
            AddGradientPass(GraphBuilder,
                            flushUniforms,
                            VertexDeclarations[static_cast<int>(
                                EVertexDeclarations::Gradient)],
                            gradiantTexture,
                            gradSpanBuffer,
                            FUint32Rect({0, 0},
                                        {
                                            kGradTextureWidth,
                                            desc.gradDataHeight,
                                        }),
                            desc.gradSpanCount);
        }

        if (desc.tessVertexSpanCount > 0)
        {
            RDG_GPU_STAT_SCOPE(GraphBuilder, STAT_RiveFlush_RiveTess);
            m_tesselationTexture.Sync(GraphBuilder,
                                      &tesselationTexture,
                                      &tessSRV);

            check(tesselationTexture);
            check(tessSRV);
            check(m_tessSpanBuffer);
            check(pathSRV);
            check(contourSRV);

            auto tessSpanBuffer = m_tessSpanBuffer->Sync(
                CommandList,
                desc.firstTessVertexSpan * sizeof(TessVertexSpan));

            auto TessPassParams =
                GraphBuilder.AllocParameters<FRiveTesselationPassParameters>();
            TessPassParams->FlushUniforms = flushUniforms;
            TessPassParams->RenderTargets[0] =
                FRenderTargetBinding(tesselationTexture,
                                     ERenderTargetLoadAction::ENoAction);
            TessPassParams->VS.GLSL_pathBuffer_raw = pathSRV;
            TessPassParams->VS.GLSL_contourBuffer_raw = contourSRV;
            TessPassParams->VS.GLSL_featherTexture_raw = featherTexture;
            TessPassParams->VS.featherSampler = m_featherSampler;

            AddTessellationPass(
                GraphBuilder,
                VertexDeclarations[static_cast<int>(
                    EVertexDeclarations::Tessellation)],
                tessSpanBuffer,
                m_tessSpanIndexBuffer,
                {{0, 0}, {kTessTextureWidth, desc.tessDataHeight}},
                desc.tessVertexSpanCount,
                TessPassParams);
        }

        // render the atlas texture if needed
        if ((desc.atlasFillBatchCount | desc.atlasStrokeBatchCount) != 0)
        {
            check(m_patchIndexBuffer);
            check(m_patchVertexBuffer);

            m_featherAtlasTexture.Sync(GraphBuilder, &atlasTexture);

            AddClearRenderTargetPass(GraphBuilder, atlasTexture);

            FUintRect viewport(0,
                               0,
                               desc.atlasContentWidth,
                               desc.atlasContentHeight);

            for (size_t i = 0; i < desc.atlasFillBatchCount; ++i)
            {
                FRiveAtlasParameters* AtlasParams =
                    GraphBuilder.AllocObject<FRiveAtlasParameters>(
                        desc.atlasFillBatches[i],
                        ShaderMap);
                AtlasParams->Viewport = viewport;
                AtlasParams->VertexDeclarationRHI =
                    VertexDeclarations[static_cast<int32>(
                        EVertexDeclarations::Atlas)];
                AtlasParams->IndexBuffer = m_patchIndexBuffer;
                AtlasParams->VertexBuffer = m_patchVertexBuffer;

                FRDGAtlasPassParameters* PassParams =
                    GraphBuilder.AllocParameters<FRDGAtlasPassParameters>();
                PassParams->FlushUniforms = flushUniforms;
                PassParams->RenderTargets[0] =
                    FRenderTargetBinding(atlasTexture,
                                         ERenderTargetLoadAction::ELoad);

                PassParams->PS.coverageAtomicBuffer = coverageUAV;
                PassParams->PS.GLSL_featherTexture_raw = featherTexture;
                PassParams->PS.featherSampler = m_featherSampler;

                PassParams->VS.baseInstance =
                    desc.atlasFillBatches[i].basePatch;
                PassParams->VS.GLSL_tessVertexTexture_raw = tessSRV;
                PassParams->VS.GLSL_pathBuffer_raw = pathSRV;
                PassParams->VS.GLSL_contourBuffer_raw = contourSRV;

                AddFeatherAtlasFillDrawPass(GraphBuilder,
                                            AtlasParams,
                                            PassParams);
            }

            for (size_t i = 0; i < desc.atlasStrokeBatchCount; ++i)
            {
                FRiveAtlasParameters* AtlasParams =
                    GraphBuilder.AllocObject<FRiveAtlasParameters>(
                        desc.atlasStrokeBatches[i],
                        ShaderMap);
                AtlasParams->Viewport = viewport;
                AtlasParams->VertexDeclarationRHI =
                    VertexDeclarations[static_cast<int32>(
                        EVertexDeclarations::Atlas)];
                AtlasParams->IndexBuffer = m_patchIndexBuffer;
                AtlasParams->VertexBuffer = m_patchVertexBuffer;

                FRDGAtlasPassParameters* PassParams =
                    GraphBuilder.AllocParameters<FRDGAtlasPassParameters>();
                PassParams->FlushUniforms = flushUniforms;
                PassParams->RenderTargets[0] =
                    FRenderTargetBinding(atlasTexture,
                                         ERenderTargetLoadAction::ELoad);

                PassParams->PS.coverageAtomicBuffer = coverageUAV;
                PassParams->PS.GLSL_featherTexture_raw = featherTexture;
                PassParams->PS.featherSampler = m_featherSampler;

                PassParams->VS.baseInstance =
                    desc.atlasStrokeBatches[i].basePatch;
                PassParams->VS.GLSL_tessVertexTexture_raw = tessSRV;
                PassParams->VS.GLSL_pathBuffer_raw = pathSRV;
                PassParams->VS.GLSL_contourBuffer_raw = contourSRV;

                AddFeatherAtlasStrokeDrawPass(GraphBuilder,
                                              AtlasParams,
                                              PassParams);
            }
        }

        bool renderDirectToRasterPipeline =
            desc.interlockMode == InterlockMode::atomics &&
            (!renderTarget->TargetTextureSupportsUAV() ||
             !(desc.combinedShaderFeatures &
               ShaderFeatures::ENABLE_ADVANCED_BLEND));

        // always load because the next draw is split into multipl render
        // passes.
        ERenderTargetLoadAction loadAction = ERenderTargetLoadAction::ELoad;

        if (desc.colorLoadAction == LoadAction::clear)
        {
            if (renderDirectToRasterPipeline)
            {
                float clearColor4f[4];
                UnpackColorToRGBA32FPremul(desc.colorClearValue, clearColor4f);
                AddClearRenderTargetPass(GraphBuilder,
                                         targetTexture,
                                         FLinearColor(clearColor4f[0],
                                                      clearColor4f[1],
                                                      clearColor4f[2],
                                                      clearColor4f[3]));
            }
            else
            {
                if (m_capabilities.bSupportsTypedUAVLoads)
                {
                    float clearColor4f[4];
                    UnpackColorToRGBA32FPremul(desc.colorClearValue,
                                               clearColor4f);
                    AddClearUAVPass(GraphBuilder,
                                    targetUAV,
                                    FVector4f(clearColor4f[0],
                                              clearColor4f[1],
                                              clearColor4f[2],
                                              clearColor4f[3]));
                }
                else
                {
                    auto Color =
                        gpu::SwizzleRiveColorToRGBAPremul(desc.colorClearValue);
                    AddClearUAVPass(GraphBuilder,
                                    targetUAV,
                                    FUintVector4(Color));
                }
            }
        }

        {
            RDG_GPU_STAT_SCOPE(GraphBuilder,
                               STAT_RiveFlush_RiveFlushRenderPass);
            for (const DrawBatch& batch : *desc.drawList)
            {
                if (batch.elementCount == 0)
                {
                    continue;
                }

                AtomicPixelPermutationDomain PixelPermutationDomain;
                AtomicVertexPermutationDomain VertexPermutationDomain;

                auto ShaderFeatures =
                    desc.interlockMode == InterlockMode::atomics
                        ? desc.combinedShaderFeatures
                        : batch.shaderFeatures;

                GetPermutationForFeatures(ShaderFeatures,
                                          batch.shaderMiscFlags,
                                          m_capabilities,
                                          PixelPermutationDomain,
                                          VertexPermutationDomain);

                FRiveCommonPassParameters* CommonPassParameters =
                    GraphBuilder.AllocObject<FRiveCommonPassParameters>(
                        batch,
                        ShaderMap);
                CommonPassParameters->VertexPermutationDomain =
                    VertexPermutationDomain;
                CommonPassParameters->PixelPermutationDomain =
                    PixelPermutationDomain;
                CommonPassParameters->Viewport =
                    FUintRect(0,
                              0,
                              renderTarget->width(),
                              renderTarget->height());
                CommonPassParameters->NeedsSourceBlending =
                    renderDirectToRasterPipeline;

                FRiveFlushPassParameters* PassParameters =
                    GraphBuilder.AllocParameters<FRiveFlushPassParameters>();

                PassParameters->FlushUniforms = flushUniforms;
                PassParameters->PS.gradSampler = m_linearSampler;
                PassParameters->PS.imageSampler = m_mipmapSampler;
                PassParameters->PS.atlasSampler = m_atlasSampler;
                PassParameters->PS.featherSampler = m_featherSampler;
                PassParameters->PS.GLSL_atlasTexture_raw = atlasTexture;
                PassParameters->PS.GLSL_featherTexture_raw = featherTexture;
                PassParameters->PS.GLSL_gradTexture_raw = gradiantTexture;
                PassParameters->PS.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                PassParameters->PS.GLSL_paintBuffer_raw = paintSRV;
                PassParameters->PS.coverageAtomicBuffer = coverageUAV;
                PassParameters->PS.clipBuffer = clipUAV;
                PassParameters->PS.colorBuffer = targetUAV;

                PassParameters->VS.GLSL_tessVertexTexture_raw = tessSRV;
                PassParameters->VS.GLSL_pathBuffer_raw = pathSRV;
                PassParameters->VS.GLSL_contourBuffer_raw = contourSRV;
                PassParameters->VS.GLSL_featherTexture_raw = featherTexture;
                PassParameters->VS.featherSampler = m_featherSampler;
                PassParameters->VS.baseInstance = 0;

                if (renderDirectToRasterPipeline)
                {
                    PassParameters->RenderTargets[0] =
                        FRenderTargetBinding(targetTexture, loadAction);
                }
                else
                {
                    PassParameters->RenderTargets.ResolveRect =
                        FResolveRect(0,
                                     0,
                                     renderTarget->width(),
                                     renderTarget->height());
                }

                switch (batch.drawType)
                {
                    case DrawType::midpointFanPatches:
                    case DrawType::midpointFanCenterAAPatches:
                    case DrawType::outerCurvePatches:
                    {
                        check(pathSRV);
                        check(paintSRV);
                        check(paintAuxSRV);
                        check(contourSRV);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::Paths)];
                        CommonPassParameters->VertexBuffers[0] =
                            m_patchVertexBuffer;
                        CommonPassParameters->IndexBuffer = m_patchIndexBuffer;

                        PassParameters->VS.baseInstance = batch.baseElement;

                        AddDrawPatchesPass(GraphBuilder,
                                           CommonPassParameters,
                                           PassParameters);
                    }
                    break;
                    case DrawType::interiorTriangulation:
                    {
                        check(triangleBuffer.IsValid());
                        check(pathSRV);
                        check(paintSRV);
                        check(paintAuxSRV);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::InteriorTriangles)];
                        CommonPassParameters->VertexBuffers[0] = triangleBuffer;

                        AddDrawInteriorTrianglesPass(GraphBuilder,
                                                     CommonPassParameters,
                                                     PassParameters);
                    }
                    break;
                    case DrawType::atlasBlit:
                        check(triangleBuffer.IsValid());
                        check(pathSRV);
                        check(paintSRV);
                        check(paintAuxSRV);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::InteriorTriangles)];
                        CommonPassParameters->VertexBuffers[0] = triangleBuffer;

                        AddDrawAtlasBlitPass(GraphBuilder,
                                             CommonPassParameters,
                                             PassParameters);
                        break;
                    case DrawType::imageRect:
                    {
                        check(paintSRV);
                        check(paintAuxSRV);
                        check(m_imageDrawUniformBuffer);

                        auto imageTexture = static_cast<const TextureRHIImpl*>(
                            batch.imageTexture);

                        auto imageDrawUniforms = m_imageDrawUniformBuffer->Sync(
                            GraphBuilder,
                            batch.imageDrawDataOffset);

                        PassParameters->VS.ImageDrawUniforms =
                            imageDrawUniforms;
                        PassParameters->PS.ImageDrawUniforms =
                            imageDrawUniforms;

                        PassParameters->PS.GLSL_imageTexture_raw =
                            imageTexture->asRDGTexture(GraphBuilder);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::ImageRect)];
                        CommonPassParameters->VertexBuffers[0] =
                            m_imageRectVertexBuffer;
                        CommonPassParameters->IndexBuffer =
                            m_imageRectIndexBuffer;

                        AddDrawImageRectPass(GraphBuilder,
                                             CommonPassParameters,
                                             PassParameters);
                    }
                    break;
                    case DrawType::imageMesh:
                    {
                        check(paintSRV);
                        check(paintAuxSRV);
                        check(m_imageDrawUniformBuffer);

                        LITE_RTTI_CAST_OR_BREAK(IndexBuffer,
                                                const RenderBufferRHIImpl*,
                                                batch.indexBuffer);
                        LITE_RTTI_CAST_OR_BREAK(VertexBuffer,
                                                const RenderBufferRHIImpl*,
                                                batch.vertexBuffer);
                        LITE_RTTI_CAST_OR_BREAK(UVBuffer,
                                                const RenderBufferRHIImpl*,
                                                batch.uvBuffer);

                        auto imageTexture = static_cast<const TextureRHIImpl*>(
                            batch.imageTexture);

                        auto IndexBufferRHI = IndexBuffer->Sync(CommandList);
                        auto VertexBufferRHI = VertexBuffer->Sync(CommandList);
                        auto UVBufferRHI = UVBuffer->Sync(CommandList);

                        auto imageDrawUniforms =
                            PassParameters->VS.ImageDrawUniforms =
                                m_imageDrawUniformBuffer->Sync(
                                    GraphBuilder,
                                    batch.imageDrawDataOffset);

                        PassParameters->VS.ImageDrawUniforms =
                            imageDrawUniforms;
                        PassParameters->PS.ImageDrawUniforms =
                            imageDrawUniforms;

                        PassParameters->PS.GLSL_imageTexture_raw =
                            imageTexture->asRDGTexture(GraphBuilder);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::ImageMesh)];
                        CommonPassParameters->VertexBuffers[0] =
                            VertexBufferRHI;
                        CommonPassParameters->VertexBuffers[1] = UVBufferRHI;
                        CommonPassParameters->IndexBuffer = IndexBufferRHI;

                        AddDrawImageMeshPass(GraphBuilder,
                                             VertexBuffer->sizeInBytes() /
                                                 sizeof(Vec2D),
                                             CommonPassParameters,
                                             PassParameters);
                    }
                    break;
                    case DrawType::atomicResolve:
                    {
                        check(paintSRV);
                        check(paintAuxSRV);

                        CommonPassParameters->VertexDeclarationRHI =
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::Resolve)];

                        AddAtomicResolvePass(GraphBuilder,
                                             CommonPassParameters,
                                             PassParameters);
                    }
                    break;
                    case DrawType::atomicInitialize:
                    case DrawType::stencilClipReset:
                        RIVE_UNREACHABLE();
                }
            }
        } // end flush render pass scope
        static const auto CVarVisualize =
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.rive.vis"));
        switch (CVarVisualize->GetInt())
        {
            case 1:
                AddBltU32ToF4Pass(
                    GraphBuilder,
                    clipTexture,
                    targetTexture,
                    FIntPoint(renderTarget->width(), renderTarget->height()));
                break;
            case 2:
                AddBltU32ToF4Pass(
                    GraphBuilder,
                    coverageTexture,
                    targetTexture,
                    FIntPoint(renderTarget->width(), renderTarget->height()));
                break;
            case 3:
                AddBltU324ToF4Pass(
                    GraphBuilder,
                    tesselationTexture,
                    targetTexture,
                    {{0, 0},
                     {kTessTextureWidth,
                      static_cast<UE::Math::TIntPoint<int>::IntType>(
                          desc.tessDataHeight)}});
                break;
            case 4:
                AddVisualizeBufferPass(
                    GraphBuilder,
                    desc.pathCount,
                    paintSRV,
                    targetTexture,
                    FIntPoint(renderTarget->width(), renderTarget->height()));
                break;
            default:
                break;
        }
    } // End Flush Event Scope

    GraphBuilder.Execute();
}
