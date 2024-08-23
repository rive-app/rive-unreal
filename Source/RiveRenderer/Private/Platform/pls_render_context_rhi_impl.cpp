#include "pls_render_context_rhi_impl.hpp"

#include "CommonRenderResources.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderGraphBuilder.h"
#include "RHIResourceUpdates.h"

#include "Shaders/ShaderPipelineManager.h"

THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_image.hpp"
#include "rive/shaders/constants.glsl"
THIRD_PARTY_INCLUDES_END

namespace rive
{
namespace pls
{

#define SYNC_BUFFER(buffer, command_list) if(buffer)buffer->Sync(command_list);
#define SYNC_UNIFORM_BUFFER(buffer, command_list, offset)if(buffer)buffer->Sync(command_list, offset);

BufferRingRHIImpl::BufferRingRHIImpl(EBufferUsageFlags flags,
size_t in_sizeInBytes) : BufferRing(in_sizeInBytes), m_flags(flags)
{
    FRHIAsyncCommandList tmpCommandList;
    FRHIResourceCreateInfo Info(TEXT("BufferRingRHIImpl_"));
    m_buffer = tmpCommandList->CreateBuffer(in_sizeInBytes,
        /*EBufferUsageFlags::Volatile |*/ flags, (bool)(flags & EBufferUsageFlags::IndexBuffer) ?  sizeof(uint16) : 0, ERHIAccess::WriteOnlyMask, Info);
}

void BufferRingRHIImpl::Sync(FRHICommandListImmediate& commandList) const
{
    auto buffer = commandList.LockBuffer(m_buffer, 0, capacityInBytes(), RLM_WriteOnly_NoOverwrite);
    memcpy(buffer, shadowBuffer(), capacityInBytes());
    commandList.UnlockBuffer(m_buffer);
}

FBufferRHIRef BufferRingRHIImpl::contents()const
{
    return m_buffer;
}

void* BufferRingRHIImpl::onMapBuffer(int bufferIdx, size_t mapSizeInBytes)
{
    return shadowBuffer();
}

void BufferRingRHIImpl::onUnmapAndSubmitBuffer(int bufferIdx, size_t mapSizeInBytes)
{
}

StructuredBufferRingRHIImpl::StructuredBufferRingRHIImpl(EBufferUsageFlags flags,
    size_t in_sizeInBytes,
    size_t elementSize) : BufferRing(in_sizeInBytes), m_elementSize(elementSize), m_flags(flags)
{
    FRHIAsyncCommandList commandList;
    FRHIResourceCreateInfo Info(TEXT("BufferRingRHIImpl_"));
    m_buffer = commandList->CreateStructuredBuffer(m_elementSize, capacityInBytes(),
        EBufferUsageFlags::Volatile | m_flags, ERHIAccess::WriteOnlyMask, Info);
    m_srv = commandList->CreateShaderResourceView(m_buffer);
}

void StructuredBufferRingRHIImpl::Sync(FRHICommandListImmediate& commandList) const
{
    auto buffer = commandList.LockBuffer(m_buffer, 0, capacityInBytes(), RLM_WriteOnly_NoOverwrite);
    memcpy(buffer, shadowBuffer(), capacityInBytes());
    commandList.UnlockBuffer(m_buffer);
}

FBufferRHIRef StructuredBufferRingRHIImpl::contents()const
{
    return m_buffer;
}

void* StructuredBufferRingRHIImpl::onMapBuffer(int bufferIdx, size_t mapSizeInBytes)
{
    return shadowBuffer();
}

void StructuredBufferRingRHIImpl::onUnmapAndSubmitBuffer(int bufferIdx, size_t mapSizeInBytes)
{
}

FShaderResourceViewRHIRef StructuredBufferRingRHIImpl::srv() const
{
    return m_srv;
}


RenderBufferRHIImpl::RenderBufferRHIImpl(RenderBufferType in_type,
                                         RenderBufferFlags in_flags, size_t in_sizeInBytes) :
    lite_rtti_override(in_type, in_flags, in_sizeInBytes),
    m_buffer(in_type == RenderBufferType::vertex ? EBufferUsageFlags::VertexBuffer : EBufferUsageFlags::IndexBuffer, in_sizeInBytes),
    m_mappedBuffer(nullptr)
{
    if(in_flags & RenderBufferFlags::mappedOnceAtInitialization)
    {
        m_mappedBuffer = m_buffer.mapBuffer(in_sizeInBytes);
    }
}

void RenderBufferRHIImpl::Sync(FRHICommandListImmediate& commandList) const
{
    m_buffer.Sync(commandList);
}

FBufferRHIRef RenderBufferRHIImpl::contents()const
{
    return m_buffer.contents();
}

void* RenderBufferRHIImpl::onMap()
{
    if(flags() & RenderBufferFlags::mappedOnceAtInitialization)
    {
        check(m_mappedBuffer);
        return m_mappedBuffer;
    }
    return m_buffer.mapBuffer(sizeInBytes());
}

void RenderBufferRHIImpl::onUnmap()
{
    if(flags() & RenderBufferFlags::mappedOnceAtInitialization)
        return;
    
    m_buffer.unmapAndSubmitBuffer();
}

class PLSTextureRHIImpl : public PLSTexture
{
public:
    PLSTextureRHIImpl(uint32_t width, uint32_t height, uint32_t mipLevelCount, const TArray<uint8>& imageDataRGBA) : 
        PLSTexture(width, height)
    {
        FRHIAsyncCommandList commandList;
        auto Desc = FRHITextureCreateDesc::Create2D(TEXT("PLSTextureRHIImpl_"), m_width, m_height, EPixelFormat::PF_B8G8R8A8);
        Desc.SetNumMips(mipLevelCount);
        m_texture = commandList->CreateTexture(Desc);
        commandList->UpdateTexture2D(m_texture, 0,
            FUpdateTextureRegion2D(0, 0, 0, 0, m_width, m_height), m_width * 4, imageDataRGBA.GetData());
    }
    virtual ~PLSTextureRHIImpl()override
    {
    }

    FTextureRHIRef contents()const
    {
        return m_texture;
    }

private:
    FTextureRHIRef  m_texture;
};

PLSRenderTargetRHI::PLSRenderTargetRHI(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTextureTarget) :
PLSRenderTarget(InTextureTarget->GetSizeX(), InTextureTarget->GetSizeY()), m_textureTarget(InTextureTarget)
{
    FRHITextureCreateDesc coverageDesc = FRHITextureCreateDesc::Create2D(TEXT("RiveAtomicCoverage"));
    coverageDesc.SetFormat(PF_R32_UINT);
    coverageDesc.SetNumMips(1);
    coverageDesc.AddFlags(ETextureCreateFlags::UAV);
    m_atomicCoverageTexture = RHICmdList.CreateTexture(coverageDesc);
    FRHITextureCreateDesc scratchColorDesc = FRHITextureCreateDesc::Create2D(TEXT("RiveAtomicCoverage"));
    scratchColorDesc.SetNumMips(1);
    scratchColorDesc.AddFlags(ETextureCreateFlags::UAV);
    scratchColorDesc.SetFormat(PF_R8G8B8A8);
    m_scratchColorTexture = RHICmdList.CreateTexture(scratchColorDesc);
    
    m_coverageUAV = RHICmdList.CreateUnorderedAccessView(m_atomicCoverageTexture);
    m_scratchColorUAV = RHICmdList.CreateUnorderedAccessView(m_scratchColorTexture);
    m_targetUAV = RHICmdList.CreateUnorderedAccessView(m_textureTarget);
    m_targetSRV = RHICmdList.CreateShaderResourceView(m_textureTarget, 0);
}

std::unique_ptr<PLSRenderContext> PLSRenderContextRHIImpl::MakeContext(FRHICommandListImmediate& CommandListImmediate)
{
    auto plsContextImpl = std::make_unique<PLSRenderContextRHIImpl>(CommandListImmediate);
    return std::make_unique<PLSRenderContext>(std::move(plsContextImpl));
}

PLSRenderContextRHIImpl::PLSRenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate) : m_tessSpanIndexData()
{
    m_platformFeatures.supportsRasterOrdering = false;

    FVertexDeclarationElementList ElementList;
    ElementList.Add(FVertexElement(0, 0, VET_Float2, 0, sizeof(Vec2D), false));
    ElementList.Add(FVertexElement(1, 0, VET_Float2, 1, sizeof(Vec2D), false));
    auto VertexDeclaration = PipelineStateCache::GetOrCreateVertexDeclaration(ElementList);
    auto ShaderMap =  GetGlobalShaderMap(GMaxRHIFeatureLevel);
    m_imageMeshPipeline = std::make_unique<ImageMeshPipeline>(VertexDeclaration,ShaderMap);
    FVertexDeclarationElementList SpanElementList;
    SpanElementList.Add(FVertexElement(0, 0, VET_UShort4, 0, sizeof(short[4]), true));
    auto SpanVertexDeclaration = PipelineStateCache::GetOrCreateVertexDeclaration(SpanElementList);
    m_gradientPipeline = std::make_unique<GradientPipeline>(SpanVertexDeclaration, ShaderMap);
    FVertexDeclarationElementList TessElementList;
    size_t tessOffset = 0;
    size_t tessStride = sizeof(pls::TessVertexSpan);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_Float4, 0, tessStride, true));
    tessOffset += sizeof(float[4]);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_Float4, 1, tessStride, true));
    tessOffset += sizeof(float[4]);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_Float4, 2, tessStride, true));
    tessOffset += sizeof(float[4]);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_UInt,3, tessStride, true));
    tessOffset += sizeof(unsigned int);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_UInt,4, tessStride, true));
    tessOffset += sizeof(unsigned int);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_UInt,5, tessStride, true));
    tessOffset += sizeof(unsigned int);
    TessElementList.Add(FVertexElement(0, tessOffset, VET_UInt,6, tessStride, true));
    
    auto TessVertexDeclaration = PipelineStateCache::GetOrCreateVertexDeclaration(TessElementList);
    m_tessPipeline = std::make_unique<TessPipeline>(TessVertexDeclaration, ShaderMap);

    m_atomicResolvePipeline = std::make_unique<AtomicResolvePipeline>(GEmptyVertexDeclaration.VertexDeclarationRHI, ShaderMap);
    
    m_testSimplePipeline = std::make_unique<TestSimplePipeline>(GEmptyVertexDeclaration.VertexDeclarationRHI, ShaderMap);
    
    m_tessSpanIndexData.Reserve(std::size(pls::kTessSpanIndices));
    m_tessSpanIndexData.AddUninitialized(std::size(pls::kTessSpanIndices));
    
    FMemory::Memcpy(&(m_tessSpanIndexData)[0], pls::kTessSpanIndices,sizeof(pls::kTessSpanIndices));
    FRHIResourceCreateInfo Info(TEXT("RiveTessIndexBuffer"), &m_tessSpanIndexData);
    m_tessSpanIndexBuffer = CommandListImmediate.CreateIndexBuffer(sizeof(uint16_t),sizeof(pls::kTessSpanIndices)
        ,EBufferUsageFlags::Static,ERHIAccess::VertexOrIndexBuffer,Info);
    
    m_linearSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, 0, 1, 0, SCF_Never>::GetRHI();
    m_mipmapSampler = TStaticSamplerState<SF_AnisotropicLinear, AM_Clamp, AM_Clamp, AM_Clamp, 0, 1, 0, SCF_Never>::GetRHI();
}

rcp<PLSRenderTargetRHI> PLSRenderContextRHIImpl::makeRenderTarget(FRHICommandListImmediate& RHICmdList,const FTexture2DRHIRef& InTargetTexture)
{
    return make_rcp<PLSRenderTargetRHI>(RHICmdList, InTargetTexture);
}

rcp<PLSTexture> PLSRenderContextRHIImpl::decodeImageTexture(Span<const uint8_t> encodedBytes)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if(!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(encodedBytes.data(), encodedBytes.size()))
    {
        return nullptr;
    }

    TArray<uint8> UncompressedBGRA;
    if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
    {
        return nullptr;
    }
    
    return make_rcp<PLSTextureRHIImpl>(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), 1, UncompressedBGRA);
}

void PLSRenderContextRHIImpl::resizeFlushUniformBuffer(size_t sizeInBytes)
{
    m_flushUniformBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_flushUniformBuffer = std::make_unique<UniformBufferRHIImpl<FFlushUniforms>>(sizeInBytes);
    }
}

void PLSRenderContextRHIImpl::resizeImageDrawUniformBuffer(size_t sizeInBytes)
{
    m_imageDrawUniformBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_imageDrawUniformBuffer = std::make_unique<UniformBufferRHIImpl<FImageDrawUniforms>>(sizeInBytes);
    }
}

void PLSRenderContextRHIImpl::resizePathBuffer(size_t sizeInBytes, pls::StorageBufferStructure structure)
{
    m_pathBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_pathBuffer = std::make_unique<StructuredBufferRingRHIImpl>(EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::ShaderResource, sizeInBytes,
            pls::StorageBufferElementSizeInBytes(structure));
    }
}

void PLSRenderContextRHIImpl::resizePaintBuffer(size_t sizeInBytes, pls::StorageBufferStructure structure)
{
    m_paintBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_paintBuffer = std::make_unique<StructuredBufferRingRHIImpl>(EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::ShaderResource, sizeInBytes, pls::StorageBufferElementSizeInBytes(structure));
    }
}

void PLSRenderContextRHIImpl::resizePaintAuxBuffer(size_t sizeInBytes, pls::StorageBufferStructure structure)
{
    m_paintAuxBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_paintAuxBuffer = std::make_unique<StructuredBufferRingRHIImpl>(EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::ShaderResource, sizeInBytes, pls::StorageBufferElementSizeInBytes(structure));
    }
}

void PLSRenderContextRHIImpl::resizeContourBuffer(size_t sizeInBytes, pls::StorageBufferStructure structure)
{
    m_contourBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_contourBuffer = std::make_unique<StructuredBufferRingRHIImpl>(EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::ShaderResource, sizeInBytes, pls::StorageBufferElementSizeInBytes(structure));
    }
}

void PLSRenderContextRHIImpl::resizeSimpleColorRampsBuffer(size_t sizeInBytes)
{
    m_simpleColorRampsBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_simpleColorRampsBuffer = std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::SourceCopy | EBufferUsageFlags::ShaderResource
            , sizeInBytes);
    }
}

void PLSRenderContextRHIImpl::resizeGradSpanBuffer(size_t sizeInBytes)
{
    m_gradSpanBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_gradSpanBuffer = std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer, sizeInBytes);
    }
}

void PLSRenderContextRHIImpl::resizeTessVertexSpanBuffer(size_t sizeInBytes)
{
    m_tessSpanBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_tessSpanBuffer = std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer, sizeInBytes);
    }
}

void PLSRenderContextRHIImpl::resizeTriangleVertexBuffer(size_t sizeInBytes)
{
    m_triangleBuffer.reset();
    if(sizeInBytes != 0)
    {
        m_triangleBuffer = std::make_unique<BufferRingRHIImpl>(EBufferUsageFlags::VertexBuffer, sizeInBytes);
    }
}

void* PLSRenderContextRHIImpl::mapFlushUniformBuffer(size_t mapSizeInBytes)
{
    return m_flushUniformBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapImageDrawUniformBuffer(size_t mapSizeInBytes)
{
    return m_imageDrawUniformBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapPathBuffer(size_t mapSizeInBytes)
{
    return m_pathBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapPaintBuffer(size_t mapSizeInBytes)
{
    return m_paintBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapPaintAuxBuffer(size_t mapSizeInBytes)
{
    return m_paintAuxBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapContourBuffer(size_t mapSizeInBytes)
{
    return m_contourBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapSimpleColorRampsBuffer(size_t mapSizeInBytes)
{
    return m_simpleColorRampsBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapGradSpanBuffer(size_t mapSizeInBytes)
{
    return m_gradSpanBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapTessVertexSpanBuffer(size_t mapSizeInBytes)
{
    return m_tessSpanBuffer->mapBuffer(mapSizeInBytes);
}

void* PLSRenderContextRHIImpl::mapTriangleVertexBuffer(size_t mapSizeInBytes)
{
    return m_triangleBuffer->mapBuffer(mapSizeInBytes);
}

void PLSRenderContextRHIImpl::unmapFlushUniformBuffer()
{
    m_flushUniformBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapImageDrawUniformBuffer()
{
    m_imageDrawUniformBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapPathBuffer()
{
    m_pathBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapPaintBuffer()
{
    m_paintBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapPaintAuxBuffer()
{
    m_paintAuxBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapContourBuffer()
{
    m_contourBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapSimpleColorRampsBuffer()
{
    m_simpleColorRampsBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapGradSpanBuffer()
{
    m_gradSpanBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapTessVertexSpanBuffer()
{
    m_tessSpanBuffer->unmapAndSubmitBuffer();
}

void PLSRenderContextRHIImpl::unmapTriangleVertexBuffer()
{
    m_triangleBuffer->unmapAndSubmitBuffer();
}

rcp<RenderBuffer> PLSRenderContextRHIImpl::makeRenderBuffer(RenderBufferType type,
                                                            RenderBufferFlags flags,
                                                            size_t sizeInBytes)
{
    if(sizeInBytes == 0)
        return nullptr;

    return make_rcp<RenderBufferRHIImpl>(type, flags, sizeInBytes);
}

void PLSRenderContextRHIImpl::resizeGradientTexture(uint32_t width, uint32_t height)
{
    check(IsInRenderingThread());
    if(width == 0 && height == 0)
    {
        m_gradiantTexture = nullptr;
        return;
    }
    else
    {
        width = std::max(width, 1u);
        height = std::max(height, 1u);
    }
    auto& commandList = GRHICommandList.GetImmediateCommandList();
    FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("riveGradientTexture"),
        {static_cast<int32_t>(width), static_cast<int32_t>(height)}, PF_R8G8B8A8);
    Desc.AddFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource);
    Desc.DetermineInititialState();
    m_gradiantTexture = commandList.CreateTexture(Desc);
}

void PLSRenderContextRHIImpl::resizeTessellationTexture(uint32_t width, uint32_t height)
{
    check(IsInRenderingThread());
    if(width == 0 && height == 0)
    {
        m_tesselationTexture = nullptr;
        return;
    }
    else
    {
        width = std::max(width, 1u);
        height = std::max(height, 1u);
    }
    
    auto& commandList = GRHICommandList.GetImmediateCommandList();
    FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("riveTessTexture"),
        {static_cast<int32_t>(width), static_cast<int32_t>(height)}, PF_R32G32B32A32_UINT);
    Desc.AddFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource);
    Desc.DetermineInititialState();
    m_tesselationTexture = commandList.CreateTexture(Desc);
}


void PLSRenderContextRHIImpl::flush(const pls::FlushDescriptor& desc)
{
    auto renderTarget = static_cast<PLSRenderTargetRHI*>(desc.renderTarget);
    //FRHICommandListImmediate* CommandList = static_cast<FRHICommandListImmediate*>(desc.externalCommandBuffer);
    check(IsInRenderingThread());
    
    FRHICommandListImmediate& CommandList = GRHICommandList.GetImmediateCommandList();
    
    SYNC_UNIFORM_BUFFER(m_flushUniformBuffer,CommandList, desc.flushUniformDataOffsetInBytes);
    SYNC_BUFFER(m_pathBuffer, CommandList);
    SYNC_BUFFER(m_paintBuffer, CommandList);
    SYNC_BUFFER(m_paintAuxBuffer, CommandList);
    SYNC_BUFFER(m_contourBuffer, CommandList);
    SYNC_BUFFER(m_simpleColorRampsBuffer, CommandList);
    SYNC_BUFFER(m_gradSpanBuffer, CommandList);
    SYNC_BUFFER(m_tessSpanBuffer, CommandList);
    SYNC_BUFFER(m_triangleBuffer, CommandList);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
    FRHIBatchedShaderParameters& BatchedShaderParameters = CommandList.GetScratchShaderParameters();

    CommandList.ClearUAVUint(renderTarget->coverageUAV(), FUintVector4(desc.coverageClearValue,0 ,0 ,0 ));
    
    if (desc.complexGradSpanCount > 0)
    {
        CommandList.Transition(FRHITransitionInfo(m_gradiantTexture, ERHIAccess::Unknown, ERHIAccess::RTV));
        GraphicsPSOInit.RenderTargetFormats[0] = m_gradiantTexture->GetFormat();
        GraphicsPSOInit.RenderTargetFlags[0] = m_gradiantTexture->GetFlags();
        GraphicsPSOInit.NumSamples = m_gradiantTexture->GetNumSamples();
        GraphicsPSOInit.RenderTargetsEnabled = 1;
        
        FRHIRenderPassInfo Info(m_gradiantTexture, ERenderTargetActions::DontLoad_Store);
        CommandList.BeginRenderPass(Info, TEXT("Rive_Render_Gradient"));
        CommandList.SetViewport(0, desc.complexGradRowsTop, 0, kGradTextureWidth, desc.complexGradRowsHeight, 1.0);
        
        m_gradientPipeline->BindShaders(CommandList, GraphicsPSOInit);
        CommandList.DrawPrimitive(0, 4, desc.complexGradSpanCount);
        
        CommandList.EndRenderPass();
        CommandList.Transition(FRHITransitionInfo(m_gradiantTexture, ERHIAccess::RTV, ERHIAccess::UAVGraphics));
    }

    if (desc.simpleGradTexelsHeight > 0)
    {
        assert(desc.simpleGradTexelsHeight * desc.simpleGradTexelsWidth * 4 <=
               simpleColorRampsBufferRing()->capacityInBytes());

        CommandList.Transition(FRHITransitionInfo(m_gradiantTexture, ERHIAccess::UAVGraphics, ERHIAccess::CopyDest));
        CommandList.UpdateFromBufferTexture2D(m_gradiantTexture, 0,
            {0, 0, 0, 0, desc.simpleGradTexelsWidth, desc.simpleGradTexelsHeight},
            kGradTextureWidth * 4, m_simpleColorRampsBuffer->contents(), desc.simpleGradDataOffsetInBytes);
        CommandList.Transition(FRHITransitionInfo(m_gradiantTexture, ERHIAccess::CopyDest, ERHIAccess::UAVGraphics));
    }

    if (desc.tessVertexSpanCount > 0)
    {
        FRHIRenderPassInfo Info(m_tesselationTexture, ERenderTargetActions::DontLoad_Store);
        CommandList.BeginRenderPass(Info, TEXT("RiveTessUpdate"));
        
        GraphicsPSOInit.RenderTargetFormats[0] = m_tesselationTexture->GetFormat();
        GraphicsPSOInit.RenderTargetFlags[0] = m_tesselationTexture->GetFlags();
        GraphicsPSOInit.NumSamples = m_tesselationTexture->GetNumSamples();
        GraphicsPSOInit.RenderTargetsEnabled = 1;
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        
        m_tessPipeline->BindShaders(CommandList, GraphicsPSOInit);
        
        UINT tessStride = sizeof(TessVertexSpan);
        UINT tessOffset = 0;
        CommandList.SetStreamSource(0, m_tessSpanBuffer->contents(), tessOffset);
        
        TessPipeline::PixelParameters PixelParameters;
        TessPipeline::VertexParameters VertexParameters;
        
        PixelParameters.FlushUniforms = m_flushUniformBuffer->contents();
        VertexParameters.FlushUniforms = m_flushUniformBuffer->contents();
        VertexParameters.GLSL_pathBuffer_raw = m_pathBuffer->srv();
        VertexParameters.GLSL_contourBuffer_raw = m_contourBuffer->srv();
        
        m_tessPipeline->SetParameters(CommandList,BatchedShaderParameters, VertexParameters, PixelParameters);
        
        CommandList.SetViewport(0, 0, 0,
            static_cast<float>(kTessTextureWidth), static_cast<float>(desc.tessDataHeight), 1);
        
        CommandList.DrawIndexedPrimitive(m_tessSpanIndexBuffer, 0, math::lossless_numeric_cast<uint32>(desc.tessVertexSpanCount),
            desc.tessVertexSpanCount, 0, desc.tessVertexSpanCount/3, desc.tessVertexSpanCount);
        CommandList.EndRenderPass();
    }
    
    FTextureRHIRef DestTexture = renderTarget->texture();
    FRHIRenderPassInfo Info(DestTexture, ERenderTargetActions::Load_Store);
    CommandList.Transition(FRHITransitionInfo(DestTexture, ERHIAccess::Unknown, ERHIAccess::RTV));
    CommandList.BeginRenderPass(Info, TEXT("Rive_Render_Flush"));
    CommandList.SetViewport(0, 0, 0, renderTarget->width(), renderTarget->height(), 1.0);
    
    GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha,BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
    GraphicsPSOInit.RenderTargetFormats[0] = DestTexture->GetFormat();
    GraphicsPSOInit.RenderTargetFlags[0] = DestTexture->GetFlags();
    GraphicsPSOInit.NumSamples = DestTexture->GetNumSamples();
    GraphicsPSOInit.RenderTargetsEnabled = 1;

    // GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
    // m_testSimplePipeline->BindShaders(CommandList,GraphicsPSOInit);
    // CommandList.DrawPrimitive(0, 2, 1);
    
    for (const DrawBatch& batch : *desc.drawList)
    {
        if (batch.elementCount == 0)
        {
            continue;
        }

        switch (batch.drawType)
        {
            case DrawType::midpointFanPatches:
                break;
            case DrawType::outerCurvePatches:
                break;
            case DrawType::interiorTriangulation:
                break;
            case DrawType::imageRect:
            case DrawType::imageMesh:
                SYNC_UNIFORM_BUFFER(m_imageDrawUniformBuffer, CommandList, batch.imageDrawDataOffset);
                GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            {
                LITE_RTTI_CAST_OR_RETURN(IndexBuffer,const rive::pls::RenderBufferRHIImpl*, batch.indexBuffer);
                LITE_RTTI_CAST_OR_RETURN(VertexBuffer,const rive::pls::RenderBufferRHIImpl*, batch.vertexBuffer);
                LITE_RTTI_CAST_OR_RETURN(UVBuffer,const rive::pls::RenderBufferRHIImpl*, batch.uvBuffer);
            
                auto imageTexture = static_cast<const PLSTextureRHIImpl*>(batch.imageTexture);
            
                SYNC_BUFFER(IndexBuffer, CommandList)
                SYNC_BUFFER(VertexBuffer, CommandList)
                SYNC_BUFFER(UVBuffer, CommandList)
                
                m_imageMeshPipeline->BindShaders(CommandList, GraphicsPSOInit);
            
                CommandList.SetStreamSource(0, VertexBuffer->contents(), 0);
                CommandList.SetStreamSource(1, UVBuffer->contents(), 0);
            
                ImageMeshPipeline::VertexParameters VertexUniforms;
                ImageMeshPipeline::PixelParameters PixelUniforms;
            
                VertexUniforms.FlushUniforms = m_flushUniformBuffer->contents();
                VertexUniforms.ImageDrawUniforms = m_imageDrawUniformBuffer->contents();
            
                PixelUniforms.FlushUniforms = m_flushUniformBuffer->contents();
                PixelUniforms.ImageDrawUniforms = m_imageDrawUniformBuffer->contents();
                
                PixelUniforms.GLSL_gradTexture_raw = m_gradiantTexture;
                PixelUniforms.GLSL_imageTexture_raw = imageTexture->contents();
                PixelUniforms.gradSampler = m_linearSampler;
                PixelUniforms.imageSampler = m_mipmapSampler;
                PixelUniforms.GLSL_paintAuxBuffer_raw = m_paintAuxBuffer->srv();
                PixelUniforms.GLSL_paintBuffer_raw = m_paintBuffer->srv();
                PixelUniforms.coverageCountBuffer = renderTarget->coverageUAV();
                //PixelUniforms.colorBuffer = renderTarget->scratchColorUAV();

                m_imageMeshPipeline->SetParameters(CommandList, BatchedShaderParameters, VertexUniforms, PixelUniforms);
                CommandList.DrawIndexedPrimitive(IndexBuffer->contents(), 0, 0,
                    VertexBuffer->sizeInBytes() / sizeof(Vec2D), 0, batch.elementCount/3, 1);
            }
               break;
            case DrawType::plsAtomicResolve:
                GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
                m_atomicResolvePipeline->BindShaders(CommandList,GraphicsPSOInit);
            {
                AtomicResolvePipeline::VertexParameters VertexParameters;
                AtomicResolvePipeline::PixelParameters PixelParameters;
                
                PixelParameters.GLSL_gradTexture_raw = m_gradiantTexture;
                PixelParameters.gradSampler = m_linearSampler;
                PixelParameters.GLSL_paintAuxBuffer_raw = m_paintAuxBuffer->srv();
                PixelParameters.GLSL_paintBuffer_raw = m_paintBuffer->srv();
                PixelParameters.coverageCountBuffer = renderTarget->coverageUAV();
                
                VertexParameters.FlushUniforms = m_flushUniformBuffer->contents();
                m_atomicResolvePipeline->SetParameters(CommandList, BatchedShaderParameters, VertexParameters, PixelParameters);
                CommandList.DrawPrimitive(0, 2, 1);
            }
                break;
            case DrawType::plsAtomicInitialize:
            case DrawType::stencilClipReset:
                RIVE_UNREACHABLE();
        }
    }
    CommandList.EndRenderPass();
    CommandList.Transition(FRHITransitionInfo(DestTexture, ERHIAccess::RTV, ERHIAccess::UAVGraphics));
}
}
}