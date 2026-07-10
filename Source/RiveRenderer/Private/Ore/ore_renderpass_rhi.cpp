/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_render_pass_rhi.hpp"
#include "Ore/ore_buffer_rhi.hpp"
#include "Ore/ore_pipeline_rhi.hpp"
#include "rive/renderer/ore/ore_context.hpp"
#include "PipelineStateCache.h"
#include "DynamicRHI.h"
#include "RHIUniformBufferLayoutInitializer.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "CommonRenderResources.h"              // GEmptyVertexDeclaration
#include "Containers/DynamicRHIResourceArray.h" // TResourceArray
#include "DataDrivenShaderPlatformInfo.h" // IsVulkanPlatform / GMaxRHIShaderPlatform
#include "Logs/RiveRendererLog.h"
#include "Ore/OreMSAAResolveShader.h"
#include "Ore/ore_bind_group_rhi.hpp"
#include "Ore/OrePlatformRHI.h" // GRHIOreNeedsReflectionSlotRemap

namespace rive::ore
{

// Size-only uniform-buffer layouts for slot-bound Ore UBOs. We bind by
// register slot (not UE's parameter-map reflection), so the layout carries
// no member or resource information — just the constant buffer size.
// Cached per size; layouts are immutable and tiny.
static const FRHIUniformBufferLayout* GetUBOLayoutForSize(uint32 AlignedSize)
{
    check(IsInRenderingThread());
    static TMap<uint32, FUniformBufferLayoutRHIRef> GLayouts;
    if (FUniformBufferLayoutRHIRef* Found = GLayouts.Find(AlignedSize))
    {
        return Found->GetReference();
    }

    const FRHIUniformBufferLayoutInitializer Initializer(TEXT("OreUBO"),
                                                         AlignedSize);
    FUniformBufferLayoutRHIRef Layout =
        RHICreateUniformBufferLayout(Initializer);
    GLayouts.Add(AlignedSize, Layout);
    return Layout.GetReference();
}

// Manual MSAA color resolve (Vulkan). Renders a fullscreen triangle that
// averages the samples of the multisample source into the single-sample dest.
// Used because UE 5.9's Vulkan dynamic-rendering auto-resolve is broken (it
// derives the resolve view from the multisample target).
static void ResolveOreMSAAColor(FRHICommandList& RHICmdList,
                                FRHITexture* Source,
                                FRHITexture* Dest)
{
    if (Source == nullptr || Dest == nullptr)
    {
        return;
    }

    FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    TShaderMapRef<FOreMSAAResolveVS> VertexShader(ShaderMap);
    TShaderMapRef<FOreMSAAResolvePS> PixelShader(ShaderMap);

    FRHIViewDesc::FTextureSRV::FInitializer SRVInit =
        FRHIViewDesc::CreateTextureSRV();
    SRVInit.SetDimensionFromTexture(Source);
    FShaderResourceViewRHIRef SourceSRV =
        RHICmdList.CreateShaderResourceView(Source, SRVInit);

    // MSAA color -> shader-read; resolve dest -> render target.
    RHICmdList.Transition({FRHITransitionInfo(Source, ERHIAccess::SRVMask),
                           FRHITransitionInfo(Dest, ERHIAccess::RTV)});

    FRHIRenderPassInfo RPInfo(Dest, ERenderTargetActions::DontLoad_Store);
    RPInfo.MultiViewCount = 0;
    RHICmdList.BeginRenderPass(RPInfo, TEXT("OreMSAAResolve"));
    {
        RHICmdList
            .SetViewport(0, 0, 0.0f, Dest->GetSizeX(), Dest->GetSizeY(), 1.0f);
        // The Ore pass may have left a scissor rect enabled; the resolve covers
        // the whole target, so disable scissoring (otherwise the resolve is
        // clipped to the last Ore scissor and only part of the target updates).
        RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

        FGraphicsPipelineStateInitializer PSOInit;
        RHICmdList.ApplyCachedRenderTargets(PSOInit);
        PSOInit.PrimitiveType = PT_TriangleList;
        PSOInit.BoundShaderState.VertexDeclarationRHI =
            GEmptyVertexDeclaration.VertexDeclarationRHI;
        PSOInit.BoundShaderState.VertexShaderRHI =
            VertexShader.GetVertexShader();
        PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
        PSOInit.RasterizerState =
            TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
        PSOInit.BlendState = TStaticBlendState<>::GetRHI();
        PSOInit.DepthStencilState =
            TStaticDepthStencilState<false, CF_Always>::GetRHI();
        SetGraphicsPipelineState(RHICmdList, PSOInit, 0);

        FRHIBatchedShaderParameters& Params =
            RHICmdList.GetScratchShaderParameters();
        SetSRVParameter(Params, PixelShader->SourceParameter, SourceSRV);
        RHICmdList.SetBatchedShaderParameters(PixelShader.GetPixelShader(),
                                              Params);

        // Fullscreen triangle: 3 vertices, 1 primitive, no vertex buffer.
        RHICmdList.DrawPrimitive(0, 1, 1);
    }
    RHICmdList.EndRenderPass();

    // Leave the resolve target readable for the downstream composite/sample.
    RHICmdList.Transition(FRHITransitionInfo(Dest, ERHIAccess::SRVMask));
}

OreRenderPassRHI::OreRenderPassRHI(Context* context,
                                   FRHICommandList& RHICmdList) :
    RenderPass(context), RHICmdList(RHICmdList)
{}

void OreRenderPassRHI::setPipeline(Pipeline* pipeline)
{
    if (!checkPipelineCompat(pipeline))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("rive::ore::OreRenderPassRHI::setPipeline pipeline not "
                    "compatible."));
        return;
    }
    auto* rhi = lite_rtti_cast<OrePipelineRHI*>(pipeline);
    rhi->m_pipelineStateInitializer.NumSamples =
        GForceOreSingleSample ? 1 : m_sampleCount;
    RHICmdList.ApplyCachedRenderTargets(rhi->m_pipelineStateInitializer);
    SetGraphicsPipelineState(RHICmdList,
                             rhi->m_pipelineStateInitializer,
                             0,
                             EApplyRendertargetOption::CheckApply,
                             true);
    // Capture this pipeline's per-stage Vulkan slot remaps for setBindGroup.
    m_vsRemap = &rhi->m_vertexSlotRemap;
    m_fsRemap = &rhi->m_fragmentSlotRemap;
    m_primitiveType = rhi->m_pipelineStateInitializer.PrimitiveType;
}

// Decides whether a resource is bound to the current stage, and at which RHI
// slot.
//
// The Ore BindingMap marks a resource visible to a stage by per-kind register
// space (D3D model), but that can over-report: a texture may carry a vertex
// slot even though the vertex shader never samples it. On D3D binding an unused
// SRV is harmless, but on flat-descriptor RHIs every kind shares one packed
// descriptor table per stage, so binding a texture to a stage that doesn't
// declare it either collides with whatever IS at that binding (e.g. the UB at
// binding 0) or overruns the table when the register exceeds its size.
//
// The compiler's reflection (captured into the per-stage remap) is the ground
// truth for what a stage actually uses. So on those RHIs we bind a resource
// only if the stage's reflection includes it, at the reflected slot; a miss
// means the stage doesn't use it -> skip. On D3D (no remap) we bind at
// the raw slot. See GRHIOreNeedsReflectionSlotRemap.
static FORCEINLINE bool ResolveBindSlot(bool bUseRemap,
                                        const TMap<uint16, uint16>* Map,
                                        uint16 Slot,
                                        uint16& OutBindSlot)
{
    if (!bUseRemap)
    {
        OutBindSlot = Slot;
        return true;
    }
    if (Map != nullptr)
    {
        if (const uint16* Found = Map->Find(Slot))
        {
            OutBindSlot = *Found;
            return true;
        }
    }
    return false; // stage's reflection doesn't include this resource
}

void OreRenderPassRHI::setVertexBuffer(uint32_t slot,
                                       Buffer* buffer,
                                       uint32_t offset)
{
    check(buffer);
    auto* rhi = lite_rtti_cast<OreBufferRHI*>(buffer);
    check(rhi && rhi->m_buffer);
    RHICmdList.SetStreamSource(slot, rhi->m_buffer, offset);
}

void OreRenderPassRHI::setIndexBuffer(Buffer* buffer,
                                      IndexFormat format,
                                      uint32_t offset)
{
    check(buffer);
    auto* rhi = lite_rtti_cast<OreBufferRHI*>(buffer);
    check(rhi && rhi->m_buffer);
    m_indexBuffer = ref_rcp(buffer);
    m_indexFormat = format;
    m_indexOffset = offset;
}

uint32 OreRenderPassRHI::GetPrimitiveCount(uint32 vertexCount) const
{
    switch (m_primitiveType)
    {
        case PT_TriangleList:
            return vertexCount / 3;
        case PT_TriangleStrip:
            return vertexCount >= 2 ? vertexCount - 2 : 0;
        case PT_LineList:
            return vertexCount / 2;
        case PT_PointList:
            return vertexCount;
        default:
            return vertexCount / 3;
    }
}

FUniformBufferRHIRef OreRenderPassRHI::ResolveUBO(
    const OreBindGroupRHI::RHIUBOBinding& ubo,
    uint32_t dynamicOffset)
{
    check(ubo.buffer != nullptr);
    const std::vector<uint8_t>& Shadow = ubo.buffer->m_shadow;
    const uint32 Offset = ubo.offset + dynamicOffset;
    if (!ensureMsgf(Offset + ubo.size <= Shadow.size(),
                    TEXT("Ore UBO bind (offset=%u, size=%u) out of range of "
                         "shadowed buffer (size=%u). Uniform buffers must be "
                         "created with BufferUsage::uniform or ::upload."),
                    Offset,
                    ubo.size,
                    static_cast<uint32>(Shadow.size())))
    {
        return nullptr;
    }

    // Constant buffers are 16-byte granular; pad the tail with zeros when
    // the bound range isn't aligned (the shader only reads ubo.size bytes).
    const uint32 AlignedSize = Align(ubo.size, 16u);
    TArray<uint8, TInlineAllocator<256>> Contents;
    Contents.SetNumZeroed(AlignedSize);
    FMemory::Memcpy(Contents.GetData(), Shadow.data() + Offset, ubo.size);

    return RHICreateUniformBuffer(Contents.GetData(),
                                  GetUBOLayoutForSize(AlignedSize),
                                  UniformBuffer_SingleDraw,
                                  EUniformBufferValidation::None);
}

void OreRenderPassRHI::setBindGroup(uint32_t groupIndex,
                                    BindGroup* inBg,
                                    const uint32_t* dynamicOffsets,
                                    uint32_t dynamicOffsetCount)
{
    auto* bg = static_cast<OreBindGroupRHI*>(inBg);
    check(bg != nullptr);
    check(groupIndex < kMaxBindGroups);

    // Hold a strong reference so the BindGroup stays alive until finish().
    m_boundGroups[groupIndex] = ref_rcp(bg);

    // On flat-descriptor RHIs (Vulkan) and bindless RHIs (Metal MSC), bind by
    // the compiler's per-stage reflection (the remap) — it gives the descriptor
    // index on Vulkan and the bindless root-constant offset on Metal; on
    // register-binding RHIs (D3D) bind by raw register slot. Vulkan is detected
    // in-tree; other such RHIs set the global. See ResolveBindSlot,
    // GRHIOreNeedsReflectionSlotRemap and GRHIOreNeedsBindlessParameters.
    const bool bUseRemap = IsVulkanPlatform(GMaxRHIShaderPlatform) ||
                           GRHIOreNeedsReflectionSlotRemap;

    // Resolve uniform buffers once, before the per-stage emit. m_ubos is
    // sorted by WGSL `@binding` at makeBindGroup time, so dynamicOffsets[i]
    // pairs with the i-th dynamic UBO in BindGroupLayout-entry order —
    // matching WebGPU semantics independently of the caller's
    // `desc.ubos[]` order.
    TArray<FUniformBufferRHIRef, TInlineAllocator<8>> UniformBuffers;
    UniformBuffers.Reserve(static_cast<int32>(bg->m_ubos.size()));
    uint32_t dynIdx = 0;
    for (const auto& ubo : bg->m_ubos)
    {
        uint32_t dynamicOffset = 0;
        if (ubo.hasDynamicOffset && dynIdx < dynamicOffsetCount)
        {
            dynamicOffset = dynamicOffsets[dynIdx++];
        }
        UniformBuffers.Add(ResolveUBO(ubo, dynamicOffset));
    }

    // The command list has a single scratch parameter batch, and
    // SetBatchedShaderParameters commits + resets it — so emit one stage
    // at a time. VS and PS register namespaces are independent (D3D11-style
    // flattening); skip the stage whose slot is kAbsent so we don't clobber
    // another resource's register.
    for (const auto Frequency : {SF_Vertex, SF_Pixel})
    {
        const bool bVertex = Frequency == SF_Vertex;
        FRHIGraphicsShader* Shader =
            bVertex ? static_cast<FRHIGraphicsShader*>(
                          RHICmdList.GetBoundVertexShader())
                    : static_cast<FRHIGraphicsShader*>(
                          RHICmdList.GetBoundPixelShader());
        if (Shader == nullptr)
        {
            // A missing pixel shader is legal (depth-only); a missing
            // vertex shader means setPipeline was never called.
            ensureMsgf(!bVertex,
                       TEXT("Ore setBindGroup requires a bound vertex "
                            "shader; call setPipeline first."));
            continue;
        }

        FRHIBatchedShaderParameters& Params =
            RHICmdList.GetScratchShaderParameters();

        // The bound pipeline's reflection remap for this stage. On Vulkan it
        // both translates register -> descriptor index AND defines membership
        // (only resources the stage actually uses appear); on Metal MSC it
        // translates register -> bindless root-constant offset the same way. On
        // D3D it's null/empty and resources bind at their raw register slot.
        const FOreVulkanSlotRemap* Remap = bVertex ? m_vsRemap : m_fsRemap;

        for (int32 i = 0; i < UniformBuffers.Num(); ++i)
        {
            const auto& ubo = bg->m_ubos[i];
            const uint16 Slot = bVertex ? ubo.vsSlot : ubo.fsSlot;
            if (Slot == BindingMap::kAbsent || !UniformBuffers[i].IsValid())
            {
                continue;
            }
            uint16 BindSlot = Slot;
            if (ResolveBindSlot(bUseRemap,
                                Remap ? &Remap->Uniform : nullptr,
                                Slot,
                                BindSlot))
            {
                Params.SetShaderUniformBuffer(BindSlot, UniformBuffers[i]);
            }
        }
        for (const auto& tex : bg->m_textures)
        {
            const uint16 Slot = bVertex ? tex.vsSlot : tex.fsSlot;
            if (Slot == BindingMap::kAbsent)
            {
                continue;
            }
            uint16 BindSlot = Slot;
            if (!ResolveBindSlot(bUseRemap,
                                 Remap ? &Remap->Texture : nullptr,
                                 Slot,
                                 BindSlot))
            {
                continue;
            }
            // Prefer the SRV — it respects the view's mip / layer range.
            // Views wrapping external textures (wrapCanvasTexture /
            // wrapRiveTexture) have no SRV; bind the texture directly.
            //
            // On bindless platforms the shader expects each resource's heap
            // handle written to its reflected root-constant offset (BindSlot
            // is that offset, from the remap), so use the bindless setters;
            // the register-slot setters would route to the non-bindless path
            // and the heap index would never be written. See
            // GRHIOreNeedsBindlessParameters.
            if (tex.srv != nullptr)
            {
                if (GRHIOreNeedsBindlessParameters)
                {
                    Params.SetBindlessResourceView(BindSlot, tex.srv);
                }
                else
                {
                    Params.SetShaderResourceViewParameter(BindSlot, tex.srv);
                }
            }
            else
            {
                if (GRHIOreNeedsBindlessParameters)
                {
                    Params.SetBindlessTexture(BindSlot, tex.rhiTexture);
                }
                else
                {
                    Params.SetShaderTexture(BindSlot, tex.rhiTexture);
                }
            }
        }
        for (const auto& samp : bg->m_samplers)
        {
            const uint16 Slot = bVertex ? samp.vsSlot : samp.fsSlot;
            if (Slot == BindingMap::kAbsent)
            {
                continue;
            }
            uint16 BindSlot = Slot;
            if (ResolveBindSlot(bUseRemap,
                                Remap ? &Remap->Sampler : nullptr,
                                Slot,
                                BindSlot))
            {
                if (GRHIOreNeedsBindlessParameters)
                {
                    Params.SetBindlessSampler(BindSlot, samp.rhiSampler);
                }
                else
                {
                    Params.SetShaderSampler(BindSlot, samp.rhiSampler);
                }
            }
        }

        RHICmdList.SetBatchedShaderParameters(Shader, Params);
    }
}

void OreRenderPassRHI::setViewport(float x,
                                   float y,
                                   float width,
                                   float height,
                                   float minDepth,
                                   float maxDepth)
{
    float MaxX = x + width;
    float MaxY = y + height;
    // Clamp to the attachment bounds: Metal rejects a viewport that exceeds the
    // render target (the Ore renderer can pass a canvas-sized rect on the
    // smaller shadow-map pass). See m_rtWidth/m_rtHeight.
    if (m_rtWidth > 0)
    {
        MaxX = FMath::Min(MaxX, static_cast<float>(m_rtWidth));
    }
    if (m_rtHeight > 0)
    {
        MaxY = FMath::Min(MaxY, static_cast<float>(m_rtHeight));
    }
    RHICmdList.SetViewport(x, y, minDepth, MaxX, MaxY, maxDepth);
}

void OreRenderPassRHI::setScissorRect(uint32_t x,
                                      uint32_t y,
                                      uint32_t width,
                                      uint32_t height)
{
    uint32 MaxX = x + width;
    uint32 MaxY = y + height;
    // Clamp to the attachment bounds. The Ore renderer can hand us a rect
    // larger than the target (e.g. a canvas-sized clip rect on the smaller
    // shadow-map pass). D3D/Vulkan clamp it silently, but Metal's validation
    // asserts
    // ("rect.y + rect.height must be <= render pass height") and drops the
    // draw, leaving the shadow map blank. See m_rtWidth/m_rtHeight.
    if (m_rtWidth > 0)
    {
        MaxX = FMath::Min(MaxX, m_rtWidth);
    }
    if (m_rtHeight > 0)
    {
        MaxY = FMath::Min(MaxY, m_rtHeight);
    }
    RHICmdList.SetScissorRect(true, x, y, MaxX, MaxY);
}

void OreRenderPassRHI::setStencilReference(uint32_t ref)
{
    RHICmdList.SetStencilRef(ref);
}

void OreRenderPassRHI::setBlendColor(float r, float g, float b, float a) {}

static uint32 PrimCount(uint32 vertexCount, uint8 primType)
{
    switch (primType)
    {
        case PT_TriangleList:
            return vertexCount / 3;
        case PT_TriangleStrip:
            return vertexCount >= 2 ? vertexCount - 2 : 0;
        case PT_LineList:
            return vertexCount / 2;
        case PT_PointList:
            return vertexCount;
        default:
            return vertexCount / 3;
    }
}

static FRHIBuffer* GetIdentityIndexBuffer(FRHICommandList& RHICmdList,
                                          uint32 NumIndices)
{
    check(IsInRenderingThread());
    static FBufferRHIRef GIdentityIndexBuffer;
    static uint32 GIdentityIndexCount = 0;
    if (NumIndices > GIdentityIndexCount)
    {
        GIdentityIndexCount =
            FMath::Max(FMath::RoundUpToPowerOfTwo(NumIndices), 2048u);
        auto* Indices = new TResourceArray<uint32>();
        Indices->AddUninitialized(GIdentityIndexCount);
        for (uint32 i = 0; i < GIdentityIndexCount; ++i)
        {
            (*Indices)[i] = i;
        }
        FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::CreateIndex<uint32>(
            TEXT("OreIdentityIndexBuffer"),
            GIdentityIndexCount);
        Desc.Usage |= EBufferUsageFlags::Static;
        Desc.SetInitActionResourceArray(Indices);
        Desc.DetermineInitialState();
        GIdentityIndexBuffer = RHICmdList.CreateBuffer(Desc);
    }
    return GIdentityIndexBuffer;
}

void OreRenderPassRHI::draw(uint32_t vertexCount,
                            uint32_t instanceCount,
                            uint32_t firstVertex,
                            uint32_t firstInstance)
{
    if (firstInstance != 0)
    {
        RHICmdList.DrawIndexedPrimitive(
            GetIdentityIndexBuffer(RHICmdList, vertexCount),
            firstVertex, // baseVertex: identity indices start at 0
            firstInstance,
            vertexCount,
            0, // firstIndex
            GetPrimitiveCount(vertexCount),
            instanceCount);
        return;
    }
    RHICmdList.DrawPrimitive(firstVertex,
                             GetPrimitiveCount(vertexCount),
                             instanceCount);
}

void OreRenderPassRHI::drawIndexed(uint32_t indexCount,
                                   uint32_t instanceCount,
                                   uint32_t firstIndex,
                                   int32_t baseVertex,
                                   uint32_t firstInstance)
{
    check(m_indexBuffer);
    auto* rhi = lite_rtti_cast<OreBufferRHI*>(m_indexBuffer.get());
    check(rhi && rhi->m_buffer);

    const uint32 IndexSize = m_indexFormat == IndexFormat::uint32 ? 4u : 2u;
    FRHIBuffer* IndexBufferRHI =
        rhi->indexBufferWithStride(RHICmdList, IndexSize);
    check(IndexBufferRHI);

    check(m_indexOffset % IndexSize == 0);
    RHICmdList.DrawIndexedPrimitive(IndexBufferRHI,
                                    baseVertex,
                                    firstInstance,
                                    indexCount,
                                    firstIndex + m_indexOffset / IndexSize,
                                    GetPrimitiveCount(indexCount),
                                    instanceCount);
}

void OreRenderPassRHI::finish()
{
    check(!m_finished);
    check(IsInRenderingThread());
    RHICmdList.EndRenderPass();

    // Manual MSAA resolve for the Vulkan path (auto-resolve is broken there).
    for (const FPendingResolve& Resolve : m_pendingResolves)
    {
        ResolveOreMSAAColor(RHICmdList, Resolve.Source, Resolve.Dest);
    }

    // Transition the color targets to a shader-read state. This inserts the
    // write->read barrier the next Ore pass (or the canvas composite) needs —
    // UE doesn't auto-synchronize these raw textures, so without it consecutive
    // passes race and only slivers of each draw survive — and leaves them
    // sampleable. beginRenderPass transitions them back to RTV before reuse.
    for (FRHITexture* ColorTarget : m_colorTargetsToRead)
    {
        if (ColorTarget != nullptr)
        {
            RHICmdList.Transition(
                FRHITransitionInfo(ColorTarget, ERHIAccess::SRVMask));
        }
    }

    if (m_depthTargetToSample != nullptr)
    {
        // Whole-resource transition (RHICmdList.Transition rejects per-plane
        // subresource transitions). beginRenderPass transitions it back to an
        // attachment layout before the next pass that renders into it.
        RHICmdList.Transition(
            FRHITransitionInfo(m_depthTargetToSample, ERHIAccess::SRVMask));
    }

#if WITH_RHI_BREADCRUMBS
    // Close the GPU debug marker opened in beginRenderPass.
    if (m_breadcrumb.IsSet())
    {
        m_breadcrumb->End(RHICmdList);
        m_breadcrumb.Reset();
    }
#endif

    m_finished = true;
}

} // namespace rive::ore
