#include "RenderContextRHIImpl.hpp"

#include "CommonRenderResources.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "PixelShaderUtils.h"
#include "RenderGraphBuilder.h"
#include "RHIResourceUpdates.h"
#include "RenderGraphUtils.h"
#include "Logs/RiveRendererLog.h"

#include "Misc/EngineVersionComparison.h"
#include "Containers/ResourceArray.h"
#include "RHIStaticStates.h"
#include "Modules/ModuleManager.h"

#include "RHICommandList.h"
#include "rive/decoders/bitmap_decoder.hpp"

THIRD_PARTY_INCLUDES_START
#include "rive/renderer/rive_render_image.hpp"
#include "rive/generated/shaders/constants.glsl.hpp"
THIRD_PARTY_INCLUDES_END

#if UE_VERSION_OLDER_THAN(5, 4, 0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip)                            \
    TStaticRasterizerState<FillMode, CullMode, false, false, DepthClip>::      \
        GetRHI()
#else // UE_VERSION_NEWER_OR_EQUAL_TO (5, 4,0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICmdList->CreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICmdList.CreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip)                            \
    TStaticRasterizerState<FillMode, CullMode, DepthClip, false>::GetRHI()
#endif

template <typename VShaderType, typename PShaderType>
void BindShaders(FRHICommandList& CommandList,
                 FGraphicsPipelineStateInitializer& GraphicsPSOInit,
                 TShaderMapRef<VShaderType> VSShader,
                 TShaderMapRef<PShaderType> PSShader,
                 FRHIVertexDeclaration* VertexDeclaration)
{
    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VSShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PSShader.GetPixelShader();
    SetGraphicsPipelineState(CommandList,
                             GraphicsPSOInit,
                             0,
                             EApplyRendertargetOption::CheckApply,
                             true,
                             EPSOPrecacheResult::Unknown);
}

template <typename ShaderType>
void SetParameters(FRHICommandList& CommandList,
                   FRHIBatchedShaderParameters& BatchedParameters,
                   TShaderMapRef<ShaderType> Shader,
                   typename ShaderType::FParameters& VParameters)
{
    ClearUnusedGraphResources(Shader, &VParameters);
    SetShaderParameters(BatchedParameters, Shader, VParameters);
    CommandList.SetBatchedShaderParameters(Shader.GetVertexShader(),
                                           BatchedParameters);
}

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
    virtual void Discard() override{};

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
    virtual void Discard() override{};

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
}

/*
 * Convenience function for adding a PF_R32 Texture visualization pass, used for
 * debuging
 */
void AddBltU32ToF4Pass(FRHICommandList& CommandList,
                       const FGlobalShaderMap* ShaderMap,
                       FTextureRHIRef SourceTexture,
                       FTextureRHIRef DestTexture,
                       FIntPoint Size)
{
    // DEBUG STUFF
    FRDGBuilder DebugBuilder(CommandList.GetAsImmediate());

    auto DebugTexture = DebugBuilder.RegisterExternalTexture(
        CreateRenderTarget(DestTexture, TEXT("DebugRenderTarget")));

    TShaderMapRef<FRiveBltU32AsF4PixelShader> PixelShader(ShaderMap);

    FRiveBltU32AsF4PixelShader::FParameters* BltU32ToF4Parameters =
        DebugBuilder.AllocParameters<FRiveBltU32AsF4PixelShader::FParameters>();

    BltU32ToF4Parameters->SourceTexture = SourceTexture;
    BltU32ToF4Parameters->RenderTargets[0] =
        FRenderTargetBinding(DebugTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(DebugBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("U32ToF4"),
                                         PixelShader,
                                         BltU32ToF4Parameters,
                                         FIntRect(0, 0, Size.X, Size.Y));
    DebugBuilder.Execute();
}

/*
 * Convenience function for adding a PF_R32G32B32A32_UINT Texture visualization
 * pass, used for debuging
 */
void AddBltU324ToF4Pass(FRHICommandList& CommandList,
                        const FGlobalShaderMap* ShaderMap,
                        FTextureRHIRef SourceTexture,
                        FTextureRHIRef DestTexture,
                        FIntPoint Size)
{
    // DEBUG STUFF
    FRDGBuilder DebugBuilder(CommandList.GetAsImmediate());

    auto DebugTexture = DebugBuilder.RegisterExternalTexture(
        CreateRenderTarget(DestTexture, TEXT("DebugRenderTarget")));

    TShaderMapRef<FRiveBltU324AsF4PixelShader> PixelShader(ShaderMap);

    FRiveBltU324AsF4PixelShader::FParameters* BltU324ToF4Parameters =
        DebugBuilder
            .AllocParameters<FRiveBltU324AsF4PixelShader::FParameters>();

    BltU324ToF4Parameters->SourceTexture = SourceTexture;
    BltU324ToF4Parameters->RenderTargets[0] =
        FRenderTargetBinding(DebugTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(DebugBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("U324ToF4"),
                                         PixelShader,
                                         BltU324ToF4Parameters,
                                         FIntRect(0, 0, Size.X, Size.Y));
    DebugBuilder.Execute();
}

/*
 * Convenience function for adding a PF_R32G32_UINT Buffer visualization pass,
 * used for debuging
 */
void AddVisualizeBufferPass(FRHICommandList& CommandList,
                            const FGlobalShaderMap* ShaderMap,
                            int32 BufferSize,
                            FShaderResourceViewRHIRef SourceBuffer,
                            FTextureRHIRef DestTexture,
                            FIntPoint Size)
{
    // DEBUG STUFF
    FRDGBuilder DebugBuilder(CommandList.GetAsImmediate());

    auto DebugTexture = DebugBuilder.RegisterExternalTexture(
        CreateRenderTarget(DestTexture, TEXT("DebugRenderTarget")));

    TShaderMapRef<FRiveVisualizeBufferPixelShader> PixelShader(ShaderMap);

    FRiveVisualizeBufferPixelShader::FParameters* VisBufferParameters =
        DebugBuilder
            .AllocParameters<FRiveVisualizeBufferPixelShader::FParameters>();
    VisBufferParameters->ViewSize = FUintVector2(Size.X, Size.Y);
    VisBufferParameters->BufferSize = BufferSize;

    VisBufferParameters->SourceBuffer = SourceBuffer;
    VisBufferParameters->RenderTargets[0] =
        FRenderTargetBinding(DebugTexture, ERenderTargetLoadAction::EClear);

    FPixelShaderUtils::AddFullscreenPass(DebugBuilder,
                                         ShaderMap,
                                         RDG_EVENT_NAME("VisBuffer"),
                                         PixelShader,
                                         VisBufferParameters,
                                         FIntRect(0, 0, Size.X, Size.Y));
    DebugBuilder.Execute();
}

RHICapabilities::RHICapabilities()
{
    bSupportsPixelShaderUAVs = GRHISupportsPixelShaderUAVs;
    bSupportsRasterOrderViews = GRHISupportsRasterOrderViews;
    bSupportsTypedUAVLoads =
        UE::PixelFormat::HasCapabilities(PF_R8G8B8A8,
                                         EPixelFormatCapabilities::UAV) &&
        UE::PixelFormat::HasCapabilities(PF_B8G8R8A8,
                                         EPixelFormatCapabilities::UAV) &&
        RHISupports4ComponentUAVReadWrite(GMaxRHIShaderPlatform);
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
    FRHIResourceCreateInfo Info(TEXT("BufferRingRHIImpl_"));

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

        auto Desc = FRHITextureCreateDesc::Create2D(TEXT("PLSTextureRHIImpl_"),
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

    TextureRHIImpl(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevelCount,
                   const TArray<uint8>& imageData,
                   EPixelFormat PixelFormat = PF_B8G8R8A8) :
        Texture(width, height)
    {
        FRHIAsyncCommandList commandList;
        FRHICommandListScopedPipelineGuard Guard(*commandList);
        // TODO: Move to Staging Buffer
        auto Desc = FRHITextureCreateDesc::Create2D(TEXT("PLSTextureRHIImpl_"),
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
            imageData.GetData());
    }

    virtual ~TextureRHIImpl() override {}

    FTextureRHIRef contents() const { return m_texture; }

private:
    FTextureRHIRef m_texture;
};

RenderTargetRHI::RenderTargetRHI(FRHICommandList& RHICmdList,
                                 const RHICapabilities& Capabilities,
                                 const FTexture2DRHIRef& InTextureTarget) :
    RenderTarget(InTextureTarget->GetSizeX(), InTextureTarget->GetSizeY()),
    m_textureTarget(InTextureTarget)
{
    FRHITextureCreateDesc coverageDesc =
        FRHITextureCreateDesc::Create2D(TEXT("RiveAtomicCoverage"),
                                        width(),
                                        height(),
                                        PF_R32_UINT);
    coverageDesc.SetNumMips(1);
    coverageDesc.AddFlags(ETextureCreateFlags::UAV);
    m_atomicCoverageTexture = CREATE_TEXTURE(RHICmdList, coverageDesc);

    // revisit this later, for now not needed
    /*FRHITextureCreateDesc scratchColorDesc =
    FRHITextureCreateDesc::Create2D(TEXT("RiveScratchColor"), width(), height(),
    PF_R8G8B8A8); scratchColorDesc.SetNumMips(1);
    scratchColorDesc.AddFlags(ETextureCreateFlags::UAV);
    m_scratchColorTexture = CREATE_TEXTURE(RHICmdList, scratchColorDesc);*/

    FRHITextureCreateDesc clipDesc =
        FRHITextureCreateDesc::Create2D(TEXT("RiveClip"),
                                        width(),
                                        height(),
                                        PF_R32_UINT);
    clipDesc.SetNumMips(1);
    clipDesc.AddFlags(ETextureCreateFlags::UAV);
    m_clipTexture = CREATE_TEXTURE(RHICmdList, clipDesc);

    m_targetTextureSupportsUAV = static_cast<bool>(
        m_textureTarget->GetDesc().Flags & ETextureCreateFlags::UAV);

    // Rive is not currently supported without UAVs.
    assert(Capabilities.bSupportsPixelShaderUAVs);

    // TODO: Lazy Load these
    if (Capabilities.bSupportsTypedUAVLoads)
        m_targetUAV = RHICmdList.CreateUnorderedAccessView(m_textureTarget);
    else
        m_targetUAV = RHICmdList.CreateUnorderedAccessView(
            m_textureTarget,
            0,
            static_cast<uint8>(EPixelFormat::PF_R32_UINT));

    m_coverageUAV =
        RHICmdList.CreateUnorderedAccessView(m_atomicCoverageTexture);
    m_clipUAV = RHICmdList.CreateUnorderedAccessView(m_clipTexture);
    // m_scratchColorUAV =
    // RHICmdList.CreateUnorderedAccessView(m_scratchColorTexture);
}

void DelayLoadedTexture::UpdateTexture(const FRHITextureCreateDesc& inDesc,
                                       bool inNeedsSRV)
{
    m_isDirty = true;
    m_needsSRV = inNeedsSRV;
    m_desc = inDesc;
}

void DelayLoadedTexture::Sync(FRHICommandList& RHICmdList)
{
    if (m_isDirty)
    {
        m_texture = CREATE_TEXTURE(RHICmdList, m_desc);
        check(m_texture);
        if (m_needsSRV)
        {
            FRHITextureSRVCreateInfo Info(0, 1, 0, 1, m_texture->GetFormat());
            m_srv = RHICmdList.CreateShaderResourceView(m_texture, Info);
            check(m_srv);
        }
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
    m_platformFeatures.supportsFragmentShaderAtomics =
        m_capabilities.bSupportsPixelShaderUAVs;
    m_platformFeatures.supportsClipPlanes = true;
    m_platformFeatures.supportsRasterOrdering =
        false; // m_capabilities.bSupportsRasterOrderViews;
    m_platformFeatures.invertOffscreenY = true;

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

    GeneratePatchBufferData(*GPatchVertices, *GPatchIndices);

    m_patchVertexBuffer =
        makeSimpleImmutableBuffer<PatchVertex>(CommandListImmediate,
                                               TEXT("RivePatchVertexBuffer"),
                                               EBufferUsageFlags::VertexBuffer,
                                               GPatchVertices);
    m_patchIndexBuffer =
        makeSimpleImmutableBuffer<uint16_t>(CommandListImmediate,
                                            TEXT("RivePatchIndexBuffer"),
                                            EBufferUsageFlags::IndexBuffer,
                                            GPatchIndices);

    m_tessSpanIndexBuffer =
        makeSimpleImmutableBuffer<uint16_t>(CommandListImmediate,
                                            TEXT("RiveTessIndexBuffer"),
                                            EBufferUsageFlags::IndexBuffer,
                                            GTessSpanIndices);

    m_imageRectVertexBuffer = makeSimpleImmutableBuffer<ImageRectVertex>(
        CommandListImmediate,
        TEXT("ImageRectVertexBuffer"),
        EBufferUsageFlags::VertexBuffer,
        GImageRectVertices);

    m_imageRectIndexBuffer =
        makeSimpleImmutableBuffer<uint16>(CommandListImmediate,
                                          TEXT("ImageRectIndexBuffer"),
                                          EBufferUsageFlags::IndexBuffer,
                                          GImageRectIndices);

    m_mipmapSampler = TStaticSamplerState<SF_Point,
                                          AM_Clamp,
                                          AM_Clamp,
                                          AM_Clamp,
                                          0,
                                          1,
                                          0,
                                          SCF_Never>::GetRHI();
    m_linearSampler = TStaticSamplerState<SF_AnisotropicLinear,
                                          AM_Clamp,
                                          AM_Clamp,
                                          AM_Clamp,
                                          0,
                                          1,
                                          0,
                                          SCF_Never>::GetRHI();
}

rcp<RenderTargetRHI> RenderContextRHIImpl::makeRenderTarget(
    FRHICommandListImmediate& RHICmdList,
    const FTexture2DRHIRef& InTargetTexture)
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

        TArray<uint8> UncompressedBGRA;
        if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
        {
            return nullptr;
        }

        return make_rcp<TextureRHIImpl>(ImageWrapper->GetWidth(),
                                        ImageWrapper->GetHeight(),
                                        1,
                                        UncompressedBGRA);
    }
    else
    {
        // WEBP Decoding, using the built in rive method
        auto bitmap = Bitmap::decode(encodedBytes.data(), encodedBytes.size());

        if (!bitmap)
        {
            RIVE_DEBUG_ERROR("Webp Decoding Failed !");
            return nullptr;
        }

        EPixelFormat PixelFormat = EPixelFormat::PF_R8G8B8A8;
        check(bitmap->pixelFormat() == Bitmap::PixelFormat::RGBA);

        return make_rcp<TextureRHIImpl>(bitmap->width(),
                                        bitmap->height(),
                                        1,
                                        bitmap->bytes(),
                                        EPixelFormat::PF_R8G8B8A8);
    }
}

void RenderContextRHIImpl::resizeFlushUniformBuffer(size_t sizeInBytes)
{
    m_flushUniformBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_flushUniformBuffer =
            std::make_unique<UniformBufferRHIImpl<FFlushUniforms>>(sizeInBytes);
    }
}

void RenderContextRHIImpl::resizeImageDrawUniformBuffer(size_t sizeInBytes)
{
    m_imageDrawUniformBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_imageDrawUniformBuffer =
            std::make_unique<UniformBufferRHIImpl<FImageDrawUniforms>>(
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

void RenderContextRHIImpl::resizeSimpleColorRampsBuffer(size_t sizeInBytes)
{
    m_simpleColorRampsBuffer.reset();
    if (sizeInBytes != 0)
    {
        m_simpleColorRampsBuffer =
            std::make_unique<HeapBufferRing>(sizeInBytes);
    }
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

void* RenderContextRHIImpl::mapSimpleColorRampsBuffer(size_t mapSizeInBytes)
{
    return m_simpleColorRampsBuffer->mapBuffer(mapSizeInBytes);
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

void RenderContextRHIImpl::unmapFlushUniformBuffer()
{
    m_flushUniformBuffer->unmapAndSubmitBuffer();
}

void RenderContextRHIImpl::unmapImageDrawUniformBuffer()
{
    m_imageDrawUniformBuffer->unmapAndSubmitBuffer();
}

void RenderContextRHIImpl::unmapPathBuffer() {}

void RenderContextRHIImpl::unmapPaintBuffer() {}

void RenderContextRHIImpl::unmapPaintAuxBuffer() {}

void RenderContextRHIImpl::unmapContourBuffer() {}

void RenderContextRHIImpl::unmapSimpleColorRampsBuffer()
{
    m_simpleColorRampsBuffer->unmapAndSubmitBuffer();
}

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

    FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(
        TEXT("riveGradientTexture"),
        {static_cast<int32_t>(width), static_cast<int32_t>(height)},
        PF_R8G8B8A8);
    Desc.AddFlags(ETextureCreateFlags::RenderTargetable |
                  ETextureCreateFlags::ShaderResource);
    Desc.SetClearValue(FClearValueBinding(FLinearColor::Red));
    Desc.DetermineInititialState();

    m_gradiantTexture.UpdateTexture(Desc);
}

void RenderContextRHIImpl::resizeTessellationTexture(uint32_t width,
                                                     uint32_t height)
{
    check(IsInRenderingThread());

    height = std::max(1u, height);

    FRHIAsyncCommandList commandList;
    FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(
        TEXT("riveTessTexture"),
        {static_cast<int32_t>(width), static_cast<int32_t>(height)},
        PF_R32G32B32A32_UINT);
    Desc.AddFlags(ETextureCreateFlags::RenderTargetable |
                  ETextureCreateFlags::ShaderResource);
    Desc.SetClearValue(FClearValueBinding::Black);
    Desc.DetermineInititialState();

    m_tesselationTexture.UpdateTexture(Desc, true);
}

void RenderContextRHIImpl::flush(const FlushDescriptor& desc)
{
    check(IsInRenderingThread());

    auto renderTarget = static_cast<RenderTargetRHI*>(desc.renderTarget);
    check(renderTarget);

    FTextureRHIRef DestTexture = renderTarget->texture();
    check(DestTexture.IsValid());

    FRHICommandList& CommandList = GRHICommandList.GetImmediateCommandList();
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    // prevent vulkan from breaking in certain scenerios
    // this comes for free with FRDGBuilder (Render Graph)
    FRHICommandListScopedPipelineGuard Guard(CommandList);

    check(m_flushUniformBuffer);
    m_flushUniformBuffer->Sync(CommandList, desc.flushUniformDataOffsetInBytes);

    m_gradiantTexture.Sync(CommandList);
    auto gradiantTexture = m_gradiantTexture.Contents();
    check(gradiantTexture.IsValid());

    m_tesselationTexture.Sync(CommandList);
    auto tesselationTexture = m_tesselationTexture.Contents();
    check(tesselationTexture.IsValid());

    auto tessSRV = m_tesselationTexture.SRV();
    check(tessSRV.IsValid());

    FShaderResourceViewRHIRef pathSRV;
    FShaderResourceViewRHIRef paintSRV;
    FShaderResourceViewRHIRef paintAuxSRV;
    FShaderResourceViewRHIRef contourSRV;

    if (desc.pathCount > 0)
    {
        pathSRV =
            m_pathBuffer.SyncSRV(CommandList, desc.firstPath, desc.pathCount);
        paintSRV =
            m_paintBuffer.SyncSRV(CommandList, desc.firstPaint, desc.pathCount);
        paintAuxSRV = m_paintAuxBuffer.SyncSRV(CommandList,
                                               desc.firstPaintAux,
                                               desc.pathCount);
    }

    if (desc.contourCount > 0)
    {
        contourSRV = m_contourBuffer.SyncSRV(CommandList,
                                             desc.firstContour,
                                             desc.contourCount);
    }

    FBufferRHIRef triangleBuffer;
    // TODO: only sync on first flush of frame
    if (m_triangleBuffer)
    {
        triangleBuffer = m_triangleBuffer->Sync(CommandList);
    }

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.RasterizerState =
        RASTER_STATE(FM_Solid, CM_None, ERasterizerDepthClipMode::DepthClamp);
    GraphicsPSOInit.DepthStencilState =
        TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
    FRHIBatchedShaderParameters& BatchedShaderParameters =
        CommandList.GetScratchShaderParameters();

    check(renderTarget->coverageUAV());
    CommandList.ClearUAVUint(renderTarget->coverageUAV(),
                             FUintVector4(desc.coverageClearValue,
                                          desc.coverageClearValue,
                                          desc.coverageClearValue,
                                          desc.coverageClearValue));
    if (desc.combinedShaderFeatures & gpu::ShaderFeatures::ENABLE_CLIPPING)
    {
        CommandList.ClearUAVUint(renderTarget->clipUAV(), FUintVector4(0));
    }

    if (desc.complexGradSpanCount > 0)
    {
        check(m_gradSpanBuffer);
        auto gradSpanBuffer = m_gradSpanBuffer->Sync(CommandList,
                                                     desc.firstComplexGradSpan *
                                                         sizeof(GradientSpan));
        CommandList.Transition(FRHITransitionInfo(gradiantTexture,
                                                  ERHIAccess::Unknown,
                                                  ERHIAccess::RTV));
        GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

        FRHIRenderPassInfo Info(gradiantTexture,
                                ERenderTargetActions::Clear_Store);
        CommandList.BeginRenderPass(Info, TEXT("Rive_Render_Gradient"));
        CommandList.SetViewport(0,
                                desc.complexGradRowsTop,
                                0,
                                kGradTextureWidth,
                                desc.complexGradRowsTop +
                                    desc.complexGradRowsHeight,
                                1.0);
        CommandList.ApplyCachedRenderTargets(GraphicsPSOInit);

        TShaderMapRef<FRiveGradientVertexShader> VertexShader(ShaderMap);
        TShaderMapRef<FRiveGradientPixelShader> PixelShader(ShaderMap);

        BindShaders(CommandList,
                    GraphicsPSOInit,
                    VertexShader,
                    PixelShader,
                    VertexDeclarations[static_cast<int32>(
                        EVertexDeclarations::Gradient)]);

        FRiveGradientVertexShader::FParameters VertexParameters;
        FRiveGradientPixelShader::FParameters PixelParameters;

        VertexParameters.FlushUniforms = m_flushUniformBuffer->contents();
        PixelParameters.FlushUniforms = m_flushUniformBuffer->contents();

        SetParameters(CommandList,
                      BatchedShaderParameters,
                      VertexShader,
                      VertexParameters);
        SetParameters(CommandList,
                      BatchedShaderParameters,
                      PixelShader,
                      PixelParameters);

        CommandList.SetStreamSource(0, gradSpanBuffer, 0);

        CommandList.DrawPrimitive(0, 2, desc.complexGradSpanCount);

        CommandList.EndRenderPass();
        if (desc.simpleGradTexelsHeight > 0)
            CommandList.Transition(FRHITransitionInfo(gradiantTexture,
                                                      ERHIAccess::RTV,
                                                      ERHIAccess::CopyDest));
        else
            CommandList.Transition(FRHITransitionInfo(gradiantTexture,
                                                      ERHIAccess::RTV,
                                                      ERHIAccess::SRVGraphics));
    }

    if (desc.simpleGradTexelsHeight > 0)
    {
        assert(desc.simpleGradTexelsHeight * desc.simpleGradTexelsWidth * 4 <=
               simpleColorRampsBufferRing()->capacityInBytes());

        CommandList.UpdateTexture2D(gradiantTexture,
                                    0,
                                    {0,
                                     0,
                                     0,
                                     0,
                                     desc.simpleGradTexelsWidth,
                                     desc.simpleGradTexelsHeight},
                                    kGradTextureWidth * 4,
                                    m_simpleColorRampsBuffer->contents() +
                                        desc.simpleGradDataOffsetInBytes);
        CommandList.Transition(FRHITransitionInfo(gradiantTexture,
                                                  ERHIAccess::CopyDest,
                                                  ERHIAccess::SRVGraphics));
    }

    if (desc.tessVertexSpanCount > 0)
    {
        check(m_tessSpanBuffer) check(pathSRV.IsValid())
            check(contourSRV.IsValid()) auto tessSpanBuffer =
                m_tessSpanBuffer->Sync(CommandList,
                                       desc.firstTessVertexSpan *
                                           sizeof(TessVertexSpan));
        CommandList.Transition(FRHITransitionInfo(tesselationTexture,
                                                  ERHIAccess::Unknown,
                                                  ERHIAccess::RTV));

        FRHIRenderPassInfo Info(tesselationTexture,
                                ERenderTargetActions::DontLoad_Store);
        CommandList.BeginRenderPass(Info, TEXT("RiveTessUpdate"));
        CommandList.ApplyCachedRenderTargets(GraphicsPSOInit);

        GraphicsPSOInit.RasterizerState =
            RASTER_STATE(FM_Solid,
                         CM_CCW,
                         ERasterizerDepthClipMode::DepthClamp);
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;

        TShaderMapRef<FRiveTessVertexShader> VertexShader(ShaderMap);
        TShaderMapRef<FRiveTessPixelShader> PixelShader(ShaderMap);

        BindShaders(CommandList,
                    GraphicsPSOInit,
                    VertexShader,
                    PixelShader,
                    VertexDeclarations[static_cast<int32>(
                        EVertexDeclarations::Tessellation)]);

        CommandList.SetStreamSource(0, tessSpanBuffer, 0);

        FRiveTessPixelShader::FParameters PixelParameters;
        FRiveTessVertexShader::FParameters VertexParameters;

        PixelParameters.FlushUniforms = m_flushUniformBuffer->contents();
        VertexParameters.FlushUniforms = m_flushUniformBuffer->contents();
        VertexParameters.GLSL_pathBuffer_raw = pathSRV;
        VertexParameters.GLSL_contourBuffer_raw = contourSRV;

        SetParameters(CommandList,
                      BatchedShaderParameters,
                      VertexShader,
                      VertexParameters);
        SetParameters(CommandList,
                      BatchedShaderParameters,
                      PixelShader,
                      PixelParameters);

        CommandList.SetViewport(0,
                                0,
                                0,
                                static_cast<float>(kTessTextureWidth),
                                static_cast<float>(desc.tessDataHeight),
                                1);

        CommandList.DrawIndexedPrimitive(m_tessSpanIndexBuffer,
                                         0,
                                         0,
                                         8,
                                         0,
                                         std::size(kTessSpanIndices) / 3,
                                         desc.tessVertexSpanCount);
        CommandList.EndRenderPass();
        CommandList.Transition(FRHITransitionInfo(tesselationTexture,
                                                  ERHIAccess::RTV,
                                                  ERHIAccess::SRVGraphics));
    }

    bool renderDirectToRasterPipeline =
        desc.interlockMode == InterlockMode::atomics &&
        (!renderTarget->TargetTextureSupportsUAV() ||
         !(desc.combinedShaderFeatures &
           ShaderFeatures::ENABLE_ADVANCED_BLEND));

    bool needsBarrierBeforeNextDraw = false;
    if (desc.interlockMode == InterlockMode::atomics &&
        !m_capabilities.bSupportsRasterOrderViews)
    {
        needsBarrierBeforeNextDraw = true;
    }

    // we never choose ERenderTargetActions::Clear_Store because we can not
    // change the clear color without re-creating the texture. So instead we use
    // a render graph to draw a clear color directly to it or clear it as a uav,
    // depending on if it supports uavs or not
    ERenderTargetActions loadAction =
        desc.colorLoadAction == LoadAction::dontCare
            ? ERenderTargetActions::DontLoad_Store
            : ERenderTargetActions::Load_Store;
    bool shouldPreserveRenderTarget =
        desc.colorLoadAction != LoadAction::dontCare;

    CommandList.Transition(FRHITransitionInfo(
        DestTexture,
        shouldPreserveRenderTarget ? ERHIAccess::UAVGraphics
                                   : ERHIAccess::Unknown,
        renderDirectToRasterPipeline ? ERHIAccess::RTV
                                     : ERHIAccess::UAVGraphics));

    if (desc.colorLoadAction == LoadAction::clear)
    {
        if (renderDirectToRasterPipeline)
        {
            // this will all be moved out and used everywhere once RDG is setup
            // correctly this has to be done because we have no way to update
            // the bound clear color without creating a new texture
            float clearColor4f[4];
            UnpackColorToRGBA32FPremul(desc.clearColor, clearColor4f);
            FRDGBuilder ClearPassBuilder(CommandList.GetAsImmediate());
            auto DestRDGTexture = ClearPassBuilder.RegisterExternalTexture(
                CreateRenderTarget(DestTexture, TEXT("RDG Clear Pass")));
            AddClearRenderTargetPass(ClearPassBuilder,
                                     DestRDGTexture,
                                     FLinearColor(clearColor4f[0],
                                                  clearColor4f[1],
                                                  clearColor4f[2],
                                                  clearColor4f[3]));
            ClearPassBuilder.Execute();
        }
        else
        {
            if (m_capabilities.bSupportsTypedUAVLoads)
            {
                float clearColor4f[4];
                UnpackColorToRGBA32FPremul(desc.clearColor, clearColor4f);
                CommandList.ClearUAVFloat(renderTarget->targetUAV(),
                                          FVector4f(clearColor4f[0],
                                                    clearColor4f[1],
                                                    clearColor4f[2],
                                                    clearColor4f[3]));
            }
            else
            {
                auto Color = gpu::SwizzleRiveColorToRGBAPremul(desc.clearColor);
                CommandList.ClearUAVUint(renderTarget->targetUAV(),
                                         FUintVector4(Color));
            }
        }
    }

    FRHIRenderPassInfo Info;
    if (renderDirectToRasterPipeline)
    {
        Info.ColorRenderTargets[0].RenderTarget = DestTexture;
        Info.ColorRenderTargets[0].Action = loadAction;
    }
    else
    {
        Info.ResolveRect =
            FResolveRect(0, 0, renderTarget->width(), renderTarget->height());
    }

    TArray<FRHITransitionInfo> TransitionInfos = {
        FRHITransitionInfo(renderTarget->coverageUAV(),
                           ERHIAccess::UAVGraphics,
                           ERHIAccess::UAVGraphics)};

    if (renderDirectToRasterPipeline)
        TransitionInfos.Add(
            FRHITransitionInfo(DestTexture, ERHIAccess::RTV, ERHIAccess::RTV));
    else
        TransitionInfos.Add(FRHITransitionInfo(renderTarget->targetUAV(),
                                               ERHIAccess::UAVGraphics,
                                               ERHIAccess::UAVGraphics));

    if (desc.combinedShaderFeatures & ShaderFeatures::ENABLE_CLIPPING)
        TransitionInfos.Add(FRHITransitionInfo(renderTarget->clipUAV(),
                                               ERHIAccess::UAVGraphics,
                                               ERHIAccess::UAVGraphics));

    CommandList.BeginRenderPass(Info, TEXT("Rive_Render_Flush"));
    CommandList.SetViewport(0,
                            0,
                            0,
                            renderTarget->width(),
                            renderTarget->height(),
                            1.0);

    if (renderDirectToRasterPipeline)
        GraphicsPSOInit.BlendState =
            TStaticBlendState<CW_RGBA,
                              BO_Add,
                              BF_One,
                              BF_InverseSourceAlpha,
                              BO_Add,
                              BF_One,
                              BF_InverseSourceAlpha>::GetRHI();
    // otherwise no blend
    else
        GraphicsPSOInit.BlendState = TStaticBlendState<CW_NONE>::CreateRHI();

    GraphicsPSOInit.RasterizerState =
        GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
    CommandList.ApplyCachedRenderTargets(GraphicsPSOInit);

    for (const DrawBatch& batch : *desc.drawList)
    {
        if (batch.elementCount == 0)
        {
            continue;
        }

        AtomicPixelPermutationDomain PixelPermutationDomain;
        AtomicVertexPermutationDomain VertexPermutationDomain;

        auto ShaderFeatures = desc.interlockMode == InterlockMode::atomics
                                  ? desc.combinedShaderFeatures
                                  : batch.shaderFeatures;

        GetPermutationForFeatures(ShaderFeatures,
                                  m_capabilities,
                                  PixelPermutationDomain,
                                  VertexPermutationDomain);

        if (needsBarrierBeforeNextDraw)
        {
            CommandList.Transition(TransitionInfos);
        }

        switch (batch.drawType)
        {
            case DrawType::midpointFanPatches:
            case DrawType::outerCurvePatches:
            {
                check(pathSRV.IsValid()) check(paintSRV.IsValid())
                    check(paintAuxSRV.IsValid()) check(contourSRV.IsValid())

                        GraphicsPSOInit.RasterizerState =
                    GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
                GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

                TShaderMapRef<FRivePathVertexShader> VertexShader(
                    ShaderMap,
                    VertexPermutationDomain);
                TShaderMapRef<FRivePathPixelShader> PixelShader(
                    ShaderMap,
                    PixelPermutationDomain);

                BindShaders(CommandList,
                            GraphicsPSOInit,
                            VertexShader,
                            PixelShader,
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::Paths)]);

                FRivePathPixelShader::FParameters PixelParameters;
                FRivePathVertexShader::FParameters VertexParameters;

                PixelParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();
                VertexParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();

                PixelParameters.gradSampler = m_linearSampler;
                PixelParameters.GLSL_gradTexture_raw = gradiantTexture;
                PixelParameters.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                PixelParameters.GLSL_paintBuffer_raw = paintSRV;
                PixelParameters.coverageAtomicBuffer =
                    renderTarget->coverageUAV();
                PixelParameters.clipBuffer = renderTarget->clipUAV();
                PixelParameters.colorBuffer = renderTarget->targetUAV();
                VertexParameters.GLSL_tessVertexTexture_raw = tessSRV;
                VertexParameters.GLSL_pathBuffer_raw = pathSRV;
                VertexParameters.GLSL_contourBuffer_raw = contourSRV;
                VertexParameters.baseInstance = batch.baseElement;

                SetParameters(CommandList,
                              BatchedShaderParameters,
                              VertexShader,
                              VertexParameters);
                SetParameters(CommandList,
                              BatchedShaderParameters,
                              PixelShader,
                              PixelParameters);

                CommandList.SetStreamSource(0, m_patchVertexBuffer, 0);
                CommandList.DrawIndexedPrimitive(
                    m_patchIndexBuffer,
                    0,
                    batch.baseElement,
                    kPatchVertexBufferCount,
                    PatchBaseIndex(batch.drawType),
                    PatchIndexCount(batch.drawType) / 3,
                    batch.elementCount);
            }
            break;
            case DrawType::interiorTriangulation:
            {
                check(triangleBuffer.IsValid());
                check(pathSRV.IsValid()) check(paintSRV.IsValid())
                    check(paintAuxSRV.IsValid())

                        GraphicsPSOInit.RasterizerState =
                    GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
                GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

                TShaderMapRef<FRiveInteriorTrianglesVertexShader> VertexShader(
                    ShaderMap,
                    VertexPermutationDomain);
                TShaderMapRef<FRiveInteriorTrianglesPixelShader> PixelShader(
                    ShaderMap,
                    PixelPermutationDomain);

                BindShaders(CommandList,
                            GraphicsPSOInit,
                            VertexShader,
                            PixelShader,
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::InteriorTriangles)]);

                FRiveInteriorTrianglesVertexShader::FParameters
                    VertexParameters;
                FRiveInteriorTrianglesPixelShader::FParameters PixelParameters;

                PixelParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();
                VertexParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();

                PixelParameters.gradSampler = m_linearSampler;
                PixelParameters.GLSL_gradTexture_raw = gradiantTexture;
                PixelParameters.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                PixelParameters.GLSL_paintBuffer_raw = paintSRV;
                PixelParameters.coverageAtomicBuffer =
                    renderTarget->coverageUAV();
                PixelParameters.clipBuffer = renderTarget->clipUAV();
                PixelParameters.colorBuffer = renderTarget->targetUAV();
                VertexParameters.GLSL_pathBuffer_raw = pathSRV;

                SetParameters(CommandList,
                              BatchedShaderParameters,
                              VertexShader,
                              VertexParameters);
                SetParameters(CommandList,
                              BatchedShaderParameters,
                              PixelShader,
                              PixelParameters);

                CommandList.SetStreamSource(0, triangleBuffer, 0);
                CommandList.DrawPrimitive(batch.baseElement,
                                          batch.elementCount / 3,
                                          1);
            }
            break;
            case DrawType::imageRect:
                m_imageDrawUniformBuffer->Sync(CommandList,
                                               batch.imageDrawDataOffset);
                {
                    check(paintSRV.IsValid()) check(paintAuxSRV.IsValid())

                        GraphicsPSOInit.RasterizerState =
                        RASTER_STATE(FM_Solid,
                                     CM_None,
                                     ERasterizerDepthClipMode::DepthClamp);
                    GraphicsPSOInit.PrimitiveType =
                        EPrimitiveType::PT_TriangleList;

                    TShaderMapRef<FRiveImageRectVertexShader> VertexShader(
                        ShaderMap,
                        VertexPermutationDomain);
                    TShaderMapRef<FRiveImageRectPixelShader> PixelShader(
                        ShaderMap,
                        PixelPermutationDomain);

                    BindShaders(CommandList,
                                GraphicsPSOInit,
                                VertexShader,
                                PixelShader,
                                VertexDeclarations[static_cast<int32>(
                                    EVertexDeclarations::ImageRect)]);

                    auto imageTexture =
                        static_cast<const TextureRHIImpl*>(batch.imageTexture);

                    FRiveImageRectVertexShader::FParameters VertexParameters;
                    FRiveImageRectPixelShader::FParameters PixelParameters;

                    VertexParameters.FlushUniforms =
                        m_flushUniformBuffer->contents();
                    VertexParameters.ImageDrawUniforms =
                        m_imageDrawUniformBuffer->contents();

                    PixelParameters.FlushUniforms =
                        m_flushUniformBuffer->contents();
                    PixelParameters.ImageDrawUniforms =
                        m_imageDrawUniformBuffer->contents();

                    PixelParameters.GLSL_gradTexture_raw = gradiantTexture;
                    PixelParameters.GLSL_imageTexture_raw =
                        imageTexture->contents();
                    PixelParameters.gradSampler = m_linearSampler;
                    PixelParameters.imageSampler = m_mipmapSampler;
                    PixelParameters.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                    PixelParameters.GLSL_paintBuffer_raw = paintSRV;
                    PixelParameters.coverageAtomicBuffer =
                        renderTarget->coverageUAV();
                    PixelParameters.clipBuffer = renderTarget->clipUAV();
                    PixelParameters.colorBuffer = renderTarget->targetUAV();

                    SetParameters(CommandList,
                                  BatchedShaderParameters,
                                  VertexShader,
                                  VertexParameters);
                    SetParameters(CommandList,
                                  BatchedShaderParameters,
                                  PixelShader,
                                  PixelParameters);

                    CommandList.SetStreamSource(0, m_imageRectVertexBuffer, 0);
                    CommandList.DrawIndexedPrimitive(
                        m_imageRectIndexBuffer,
                        0,
                        0,
                        std::size(kImageRectVertices),
                        0,
                        std::size(kImageRectIndices) / 3,
                        1);
                }
                break;
            case DrawType::imageMesh:
            {
                check(paintSRV.IsValid()) check(paintAuxSRV.IsValid())

                    m_imageDrawUniformBuffer->Sync(CommandList,
                                                   batch.imageDrawDataOffset);
                GraphicsPSOInit.RasterizerState =
                    RASTER_STATE(FM_Solid,
                                 CM_None,
                                 ERasterizerDepthClipMode::DepthClamp);
                GraphicsPSOInit.PrimitiveType = PT_TriangleList;

                LITE_RTTI_CAST_OR_BREAK(IndexBuffer,
                                        const RenderBufferRHIImpl*,
                                        batch.indexBuffer);
                LITE_RTTI_CAST_OR_BREAK(VertexBuffer,
                                        const RenderBufferRHIImpl*,
                                        batch.vertexBuffer);
                LITE_RTTI_CAST_OR_BREAK(UVBuffer,
                                        const RenderBufferRHIImpl*,
                                        batch.uvBuffer);

                auto imageTexture =
                    static_cast<const TextureRHIImpl*>(batch.imageTexture);

                auto IndexBufferRHI = IndexBuffer->Sync(CommandList);
                auto VertexBufferRHI = VertexBuffer->Sync(CommandList);
                auto UVBufferRHI = UVBuffer->Sync(CommandList);

                TShaderMapRef<FRiveImageMeshVertexShader> VertexShader(
                    ShaderMap,
                    VertexPermutationDomain);
                TShaderMapRef<FRiveImageMeshPixelShader> PixelShader(
                    ShaderMap,
                    PixelPermutationDomain);

                BindShaders(CommandList,
                            GraphicsPSOInit,
                            VertexShader,
                            PixelShader,
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::ImageMesh)]);

                CommandList.SetStreamSource(0, VertexBufferRHI, 0);
                CommandList.SetStreamSource(1, UVBufferRHI, 0);

                FRiveImageMeshVertexShader::FParameters VertexParameters;
                FRiveImageMeshPixelShader::FParameters PixelParameters;

                VertexParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();
                VertexParameters.ImageDrawUniforms =
                    m_imageDrawUniformBuffer->contents();

                PixelParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();
                PixelParameters.ImageDrawUniforms =
                    m_imageDrawUniformBuffer->contents();

                PixelParameters.GLSL_gradTexture_raw = gradiantTexture;
                PixelParameters.GLSL_imageTexture_raw =
                    imageTexture->contents();
                PixelParameters.gradSampler = m_linearSampler;
                PixelParameters.imageSampler = m_mipmapSampler;
                PixelParameters.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                PixelParameters.GLSL_paintBuffer_raw = paintSRV;
                PixelParameters.coverageAtomicBuffer =
                    renderTarget->coverageUAV();
                PixelParameters.clipBuffer = renderTarget->clipUAV();
                PixelParameters.colorBuffer = renderTarget->targetUAV();

                SetParameters(CommandList,
                              BatchedShaderParameters,
                              VertexShader,
                              VertexParameters);
                SetParameters(CommandList,
                              BatchedShaderParameters,
                              PixelShader,
                              PixelParameters);

                CommandList.DrawIndexedPrimitive(IndexBufferRHI,
                                                 0,
                                                 0,
                                                 VertexBuffer->sizeInBytes() /
                                                     sizeof(Vec2D),
                                                 0,
                                                 batch.elementCount / 3,
                                                 1);
            }
            break;
            case DrawType::atomicResolve:
            {
                check(paintSRV.IsValid()) check(paintAuxSRV.IsValid())

                    GraphicsPSOInit.RasterizerState =
                    RASTER_STATE(FM_Solid,
                                 CM_None,
                                 ERasterizerDepthClipMode::DepthClamp);
                GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

                TShaderMapRef<FRiveAtomicResolveVertexShader> VertexShader(
                    ShaderMap,
                    VertexPermutationDomain);
                TShaderMapRef<FRiveAtomicResolvePixelShader> PixelShader(
                    ShaderMap,
                    PixelPermutationDomain);

                BindShaders(CommandList,
                            GraphicsPSOInit,
                            VertexShader,
                            PixelShader,
                            VertexDeclarations[static_cast<int32>(
                                EVertexDeclarations::Resolve)]);

                FRiveAtomicResolveVertexShader::FParameters VertexParameters;
                FRiveAtomicResolvePixelShader::FParameters PixelParameters;

                PixelParameters.GLSL_gradTexture_raw = gradiantTexture;
                PixelParameters.gradSampler = m_linearSampler;
                PixelParameters.GLSL_paintAuxBuffer_raw = paintAuxSRV;
                PixelParameters.GLSL_paintBuffer_raw = paintSRV;
                PixelParameters.coverageAtomicBuffer =
                    renderTarget->coverageUAV();
                PixelParameters.clipBuffer = renderTarget->clipUAV();
                PixelParameters.colorBuffer = renderTarget->targetUAV();

                VertexParameters.FlushUniforms =
                    m_flushUniformBuffer->contents();

                SetParameters(CommandList,
                              BatchedShaderParameters,
                              VertexShader,
                              VertexParameters);
                SetParameters(CommandList,
                              BatchedShaderParameters,
                              PixelShader,
                              PixelParameters);

                CommandList.DrawPrimitive(0, 2, 1);
            }
            break;
            case DrawType::atomicInitialize:
            case DrawType::stencilClipReset:
                RIVE_UNREACHABLE();
        }

        needsBarrierBeforeNextDraw =
            desc.interlockMode == gpu::InterlockMode::atomics &&
            batch.needsBarrier;
    }

    CommandList.EndRenderPass();

    static const auto CVarVisualize =
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.rive.vis"));
    switch (CVarVisualize->GetInt())
    {
        case 1:
            AddBltU32ToF4Pass(
                CommandList,
                ShaderMap,
                renderTarget->clipTexture(),
                DestTexture,
                FIntPoint(renderTarget->width(), renderTarget->height()));
            break;
        case 2:
            AddBltU32ToF4Pass(
                CommandList,
                ShaderMap,
                renderTarget->coverageTexture(),
                DestTexture,
                FIntPoint(renderTarget->width(), renderTarget->height()));
            break;
        case 3:
            AddBltU324ToF4Pass(
                CommandList,
                ShaderMap,
                tesselationTexture,
                DestTexture,
                FIntPoint(renderTarget->width(), renderTarget->height()));
            break;
        case 4:
            AddVisualizeBufferPass(
                CommandList,
                ShaderMap,
                desc.pathCount,
                paintSRV,
                DestTexture,
                FIntPoint(renderTarget->width(), renderTarget->height()));
            break;
        default:
            break;
    }

    // According to Ryan at GeoTech, Unreal expects us to give the texture back
    // in UAVGraphics
    // TODO: Check that this is true
    if (renderDirectToRasterPipeline)
    {
        CommandList.Transition(FRHITransitionInfo(DestTexture,
                                                  ERHIAccess::RTV,
                                                  ERHIAccess::UAVGraphics));
    }
    else
    {
        CommandList.Transition(FRHITransitionInfo(DestTexture,
                                                  ERHIAccess::UAVGraphics,
                                                  ERHIAccess::UAVGraphics));
    }
}