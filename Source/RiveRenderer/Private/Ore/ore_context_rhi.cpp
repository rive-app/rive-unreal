/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_context_rhi.hpp"

#include "Ore/ore_buffer_rhi.hpp"
#include "Ore/ore_texture_rhi.hpp"
#include "Ore/ore_sampler_rhi.hpp"
#include "Ore/ore_shader_module_rhi.hpp"
#include "Ore/ore_pipeline_rhi.hpp"
#include "Ore/ore_bind_group_layout_rhi.hpp"
#include "Ore/ore_bind_group_rhi.hpp"
#include "Ore/ore_render_pass_rhi.hpp"

#include "RenderGraphResources.h"
#include "DataDrivenShaderPlatformInfo.h" // IsVulkanPlatform / GMaxRHIShaderPlatform
#include "Platform/RenderContextRHIImpl.hpp"
#include "Ore/RiveOrderShaderHandler.h"
#include "Ore/OreHelpers.hpp"
#include "Serialization/PackageWriter.h"

#include "TextureRHIImpl.hpp"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/render_canvas.hpp"
#include "rive/rive_types.hpp"
#include <Ore/RiveOreShaderType.h>
THIRD_PARTY_INCLUDES_END

#include <algorithm>

class FOreBufferResourceArray : public FResourceArrayInterface
{
public:
    FOreBufferResourceArray(const rive::ore::BufferDesc& desc) :
        m_size(desc.size),
        m_allowCPUAccess(!desc.immutable),
        m_isStatic(desc.immutable)
    {
        m_data.reset(new uint8_t[m_size]);
        memcpy(m_data.get(), desc.data, m_size);
    }

    [[nodiscard]] virtual const void* GetResourceData() const override
    {
        return m_data.get();
    }

    [[nodiscard]] virtual uint32 GetResourceDataSize() const override
    {
        return m_size;
    }

    virtual void Discard() override { m_data.reset(nullptr); }

    [[nodiscard]] virtual bool IsStatic() const override { return m_isStatic; }

    [[nodiscard]] virtual bool GetAllowCPUAccess() const override
    {
        return m_allowCPUAccess;
    }

    virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override
    {
        m_allowCPUAccess = bInNeedsCPUAccess;
    }

    std::unique_ptr<uint8_t[]> m_data;
    uint32_t m_size;
    bool m_allowCPUAccess;
    bool m_isStatic;
};

OreContextRHI::OreContextRHI() : Context(nullptr)
{
    rive::ore::Features& f = m_features;

    f.colorBufferFloat = GPixelFormats[PF_A32B32G32R32F].Supported &&
                         GPixelFormats[PF_FloatRGBA].Supported;
    f.colorBufferHalfFloat = GPixelFormats[PF_FloatRGBA].Supported;
    f.perTargetBlend = GSupportsSeparateRenderTargetBlendState;
    f.perTargetWriteMask = GSupportsSeparateRenderTargetBlendState;
    f.textureViewSampling = GRHISupportsTextureViews;
    f.drawBaseInstance = GRHISupportsFirstInstance;
    f.depthBiasClamp = false;
    f.anisotropicFiltering = true; // Universally supported on UE desktop RHIs.
    f.texture3D = GSupportsTexture3D;
    f.textureArrays = true; // Texture2DArray is core in all UE SM5+ RHIs.
    const bool bSM5Plus = GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5;
    f.computeShaders = bSM5Plus;
    f.storageBuffers = bSM5Plus;
    f.bc = GPixelFormats[PF_DXT1].Supported;
    f.etc2 = GPixelFormats[PF_ETC2_RGB].Supported;
    f.astc = GPixelFormats[PF_ASTC_4x4].Supported;

    f.maxColorAttachments = MaxSimultaneousRenderTargets;
    f.maxTextureSize2D = GMaxTextureDimensions;
    f.maxTextureSizeCube = GMaxCubeTextureDimensions;
    f.maxTextureSize3D = GMaxVolumeTextureDimensions;
    f.maxUniformBufferSize = 65536; // D3D constant-buffer cap; no UE global.
    f.maxVertexAttributes = MaxVertexElementCount;
    f.maxSamplers = GMaxTextureSamplers;
}

rive::rcp<rive::ore::Buffer> OreContextRHI::makeBuffer(
    const rive::ore::BufferDesc& desc)
{
    check(IsInRenderingThread());
    if (desc.immutable && desc.data == nullptr)
    {
        setLastError(
            "makeBuffer: immutable=true requires `data` (the buffer is "
            "GPU-only after creation)");
        return nullptr;
    }

    auto buffer = rive::rcp<rive::ore::OreBufferRHI>(
        new rive::ore::OreBufferRHI(desc.size, desc.usage));

    FRHIBufferCreateDesc bd = FRHIBufferCreateDesc::Create(
        UTF8_TO_TCHAR(desc.label),
        desc.size,
        desc.usage == rive::ore::BufferUsage::index ? 2 : 4,
        EBufferUsageFlags::ShaderResource);

    switch (desc.usage)
    {
        case rive::ore::BufferUsage::vertex:
            bd.Usage |= EBufferUsageFlags::VertexBuffer;
            break;
        case rive::ore::BufferUsage::index:
            bd.Usage |= EBufferUsageFlags::IndexBuffer;
            break;
        case rive::ore::BufferUsage::uniform:
            bd.Usage |= EBufferUsageFlags::UniformBuffer;
            break;
        case rive::ore::BufferUsage::upload:
            bd.Usage |= EBufferUsageFlags::SourceCopy;
            break;
    }

    if (desc.immutable)
    {
        bd.Usage |= EBufferUsageFlags::Static;
        bd.SetInitActionResourceArray(new FOreBufferResourceArray(desc));
    }
    else
    {
        bd.Usage |=
            EBufferUsageFlags::Dynamic | EBufferUsageFlags::KeepCPUAccessible;
    }

    bd.DetermineInitialState();
    FRHICommandList& CmdList = getCommandList();

    if (desc.usage != rive::ore::BufferUsage::uniform)
    {
        buffer->m_usageFlags = bd.Usage;
        buffer->m_buffer = CmdList.CreateBuffer(bd);
    }

    // Seed the CPU shadow (uniform / upload / index usage) so ResolveUBO and
    // indexBufferWithStride see initial contents that never go through
    // Buffer::update.
    if (desc.data != nullptr && !buffer->m_shadow.empty())
    {
        memcpy(buffer->m_shadow.data(), desc.data, desc.size);
    }

    return buffer;
}

rive::rcp<rive::ore::Texture> OreContextRHI::makeTexture(
    const rive::ore::TextureDesc& desc)
{
    check(IsInRenderingThread());
    auto texture =
        rive::rcp<rive::ore::OreTextureRHI>(new rive::ore::OreTextureRHI(desc));

    FRHITextureCreateDesc Desc;
    EPixelFormat format = PixelFormatFromOreFormat(desc.format);
    switch (desc.type)
    {
        case rive::ore::TextureType::texture2D:
            Desc =
                FRHITextureCreateDesc::Create2D(UTF8_TO_TCHAR(desc.label),
                                                static_cast<int32>(desc.width),
                                                static_cast<int32>(desc.height),
                                                format);
            break;
        case rive::ore::TextureType::cube:
        {
            uint32_t Size =
                FMath::Max(desc.width,
                           FMath::Max(desc.height, desc.depthOrArrayLayers));
            Desc = FRHITextureCreateDesc::CreateCube(UTF8_TO_TCHAR(desc.label),
                                                     static_cast<int32>(Size),
                                                     format);
        }
        break;
        case rive::ore::TextureType::texture3D:
            Desc = FRHITextureCreateDesc::Create3D(
                UTF8_TO_TCHAR(desc.label),
                static_cast<int32>(desc.width),
                static_cast<int32>(desc.height),
                static_cast<int32>(desc.depthOrArrayLayers),
                format);
            break;
        case rive::ore::TextureType::array2D:
            Desc = FRHITextureCreateDesc::Create2DArray(
                UTF8_TO_TCHAR(desc.label),
                static_cast<int32>(desc.width),
                static_cast<int32>(desc.height),
                static_cast<int32>(desc.depthOrArrayLayers),
                format);
            break;
    }

    Desc.AddFlags(ETextureCreateFlags::ShaderResource);

    if (FormatIsDepthOrStencil(desc.format))
    {
        // Rive clears depth to depthClearValue (default 1.0) and stencil to 0.
        // UE bakes the clear value into the texture (the render pass clears to
        // it), so it must match Rive's convention.
        Desc.SetClearValue(FClearValueBinding(1.0f, 0));
        Desc.AddFlags(ETextureCreateFlags::DepthStencilTargetable);
    }
    else if (desc.renderTarget)
    {
        Desc.SetClearValue(FClearValueBinding(FLinearColor::Transparent));
        Desc.AddFlags(ETextureCreateFlags::RenderTargetable);
    }
    else if (!GForceOreSingleSample && desc.sampleCount != 1)
    {
        Desc.AddFlags(ETextureCreateFlags::ResolveTargetable);
    }

    Desc.SetNumSamples(
        GForceOreSingleSample ? 1 : static_cast<uint8>(desc.sampleCount));
    Desc.SetNumMips(desc.numMipmaps);

    FRHICommandList& CmdList = getCommandList();
    texture->m_texture = CmdList.CreateTexture(Desc);

    return texture;
}

rive::rcp<rive::ore::TextureView> OreContextRHI::makeTextureView(
    const rive::ore::TextureViewDesc& desc)
{
    auto textureView = rive::rcp<rive::ore::OreTextureViewRHI>(
        new rive::ore::OreTextureViewRHI(rive::ref_rcp(desc.texture), desc));

    {
        auto* tex =
            lite_rtti_cast<rive::ore::OreTextureRHI*>(textureView->texture());

        // A multisample texture has no sampleable SRV in this binding model:
        // the Ore shaders sample single-sample textures, and MSAA targets are
        // resolved before they are read. An SRV of an MSAA texture must use the
        // multisample target, which UE's FRHIViewDesc can't express (there is
        // no multisample ETextureDimension) — so the only SRV we could build is
        // a non-multisample one, whose target is incompatible with the
        // underlying multisample texture. Some RHIs reject that mismatch
        // outright when the view is created. Leave m_srv null; setBindGroup
        // binds the raw texture if the resource is ever referenced (it isn't,
        // for an MSAA attachment like the depth buffer that triggers this
        // path).
        if (tex->m_texture->GetDesc().NumSamples > 1)
        {
            return textureView;
        }

        FRHICommandList& CmdList = getCommandList();
        FRHIViewDesc::FTextureSRV::FInitializer init =
            FRHIViewDesc::CreateTextureSRV();
        // Use the view desc's dimension, not the underlying texture's: a view
        // of one slice/face is a 2D view over a 2D-array/cube resource. Some
        // RHIs reject a view whose target doesn't match what the shader binds.
        // See ETextureDimensionFromOreViewDesc.
        init.SetDimension(ETextureDimensionFromOreViewDesc(desc));
        init.SetArrayRange(desc.baseLayer, desc.layerCount);
        init.SetMipRange(desc.baseMipLevel, desc.mipCount);
        const auto Format = tex->format();
        init.SetFormat(PixelFormatFromOreFormat(Format));
        // Depth/stencil views (e.g. shadow maps) must read the depth (or
        // stencil) plane so the SRV is created with the depth-readable format.
        if (desc.aspect == rive::ore::TextureAspect::depthOnly)
        {
            init.SetPlane(ERHITexturePlane::Depth);
        }
        else if (desc.aspect == rive::ore::TextureAspect::stencilOnly)
        {
            init.SetPlane(ERHITexturePlane::Stencil);
        }
        else
        {
            init.SetPlane(ERHITexturePlane::Primary);
        }

        textureView->m_srv =
            CmdList.CreateShaderResourceView(tex->m_texture, init);
    }

    return textureView;
}

rive::rcp<rive::ore::Sampler> OreContextRHI::makeSampler(
    const rive::ore::SamplerDesc& desc)
{
    check(IsInRenderingThread());
    auto sampler =
        rive::rcp<rive::ore::OreSamplerRHI>(new rive::ore::OreSamplerRHI());
    FSamplerStateInitializerRHI init = SamplerStateInitializerFromOreDesc(desc);
    sampler->m_sampler = RHICreateSamplerState(init);
    return sampler;
}

rive::rcp<rive::ore::ShaderModule> OreContextRHI::makeShaderModule(
    const rive::ore::ShaderModuleDesc& desc)
{
    check(IsInRenderingThread());
    auto shaderModule = rive::rcp<rive::ore::OreShaderModuleRHI>(
        new rive::ore::OreShaderModuleRHI());
    shaderModule->applyBindingMapFromDesc(desc);
    auto ModuleData =
        GRiveOreShaderHandler->ShaderModuleForId(desc.shaderAssetId);
    if (!ensure(ModuleData))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Failed to find shader data for module id %i"),
               desc.shaderAssetId);
        return nullptr;
    }
    const EShaderFrequency Frequency =
        desc.stage == rive::ore::ShaderStage::vertex ? SF_Vertex : SF_Pixel;
    const FRiveOreShaderBlob* Blob = ModuleData->Shaders.Find(
        {StringCast<TCHAR>(desc.hlslEntryPoint).Get(), Frequency});
    if (!ensure(Blob))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Failed to find shader for entry %hs"),
               desc.hlslEntryPoint);
        return nullptr;
    }

    // Build the RHI shader straight from the captured, UE-framed bytecode. No
    // FShader / global shader map lookup, and nothing validates the resource
    // bindings against reflection — the BindingMap drives slot binding instead.
    //
    // RHICreate*Shader(Code, Hash) ignores the Hash argument (it reads the hash
    // from the code framing, which our raw compile blob doesn't carry), so the
    // shader's hash would be zero. Set it explicitly: a packaged build logs
    // every created PSO into the PSO file cache, whose Verify() rejects a PSO
    // with a zero shader hash and asserts. The blob's hash is the compiler's
    // OutputHash, which is stable and unique per shader.
    if (Frequency == SF_Vertex)
    {
        shaderModule->VertexShader =
            RHICreateVertexShader(Blob->Code, Blob->Hash);
        if (shaderModule->VertexShader.IsValid())
        {
            shaderModule->VertexShader->SetHash(Blob->Hash);
        }
        else
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("RHICreateVertexShader returned an invalid shader for "
                        "Ore module id %i entry '%hs' (%d code bytes). The RHI "
                        "rejected the cooked bytecode framing — Ore will not "
                        "draw."),
                   desc.shaderAssetId,
                   desc.hlslEntryPoint,
                   Blob->Code.Num());
        }
    }
    else
    {
        shaderModule->PixelShader =
            RHICreatePixelShader(Blob->Code, Blob->Hash);
        if (shaderModule->PixelShader.IsValid())
        {
            shaderModule->PixelShader->SetHash(Blob->Hash);
        }
        else
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("RHICreatePixelShader returned an invalid shader for "
                        "Ore module id %i entry '%hs' (%d code bytes). The RHI "
                        "rejected the cooked bytecode framing — Ore will not "
                        "draw."),
                   desc.shaderAssetId,
                   desc.hlslEntryPoint,
                   Blob->Code.Num());
        }
    }
    // Carry the Vulkan slot remap through to bind time (empty on some backends
    // and for a Vulkan shader that declares no resources).
    shaderModule->SlotRemap = Blob->SlotRemap;
    return shaderModule;
}

rive::rcp<rive::ore::BindGroupLayout> OreContextRHI::makeBindGroupLayout(
    const rive::ore::BindGroupLayoutDesc& desc)
{
    check(IsInRenderingThread());
    if (desc.groupIndex >= rive::ore::kMaxBindGroups)
    {
        setLastError("makeBindGroupLayout: groupIndex %u out of range [0, %u)",
                     desc.groupIndex,
                     rive::ore::kMaxBindGroups);
        return nullptr;
    }
    auto bindGroupLayout = rive::rcp<rive::ore::OreBindGroupLayoutRHI>(
        new rive::ore::OreBindGroupLayoutRHI(this, desc));
    return bindGroupLayout;
}

rive::rcp<rive::ore::BindGroup> OreContextRHI::makeBindGroup(
    const rive::ore::BindGroupDesc& desc)
{
    check(IsInRenderingThread());
    if (desc.layout == nullptr)
    {
        setLastError("makeBindGroup: BindGroupDesc::layout is null");
        return nullptr;
    }
    rive::ore::BindGroupLayout* layout = desc.layout;
    const uint32_t groupIndex = layout->groupIndex();
    if (groupIndex >= rive::ore::kMaxBindGroups)
    {
        setLastError("makeBindGroup: layout->groupIndex %u out of range",
                     groupIndex);
        return nullptr;
    }

    auto bindGroup =
        rive::rcp<rive::ore::OreBindGroupRHI>(new rive::ore::OreBindGroupRHI());
    bindGroup->m_context = this;
    bindGroup->m_layoutRef = ref_rcp(layout);

    // UE's RHI binds loose resources per-stage by register slot, like D3D11's
    // independent VS / PS namespaces. The allocator's per-stage native slots
    // are populated on the layout entries by `makeLayoutFromShader`.
    auto lookupStages = [&](uint32_t binding,
                            rive::ore::BindingKind expected,
                            uint16_t* outVS,
                            uint16_t* outFS) -> bool {
        const rive::ore::BindGroupLayoutEntry* le = layout->findEntry(binding);
        if (le == nullptr)
        {
            setLastError("makeBindGroup: (group=%u, binding=%u) not declared "
                         "in BindGroupLayout",
                         groupIndex,
                         binding);
            return false;
        }
        const bool kindOK =
            le->kind == expected ||
            ((le->kind == rive::ore::BindingKind::sampler ||
              le->kind == rive::ore::BindingKind::comparisonSampler) &&
             (expected == rive::ore::BindingKind::sampler ||
              expected == rive::ore::BindingKind::comparisonSampler));
        if (!kindOK)
        {
            setLastError("makeBindGroup: (group=%u, binding=%u) layout kind "
                         "mismatch",
                         groupIndex,
                         binding);
            return false;
        }
        *outVS = (le->nativeSlotVS ==
                  rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent)
                     ? rive::ore::BindingMap::kAbsent
                     : static_cast<uint16_t>(le->nativeSlotVS);
        *outFS = (le->nativeSlotFS ==
                  rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent)
                     ? rive::ore::BindingMap::kAbsent
                     : static_cast<uint16_t>(le->nativeSlotFS);
        if (*outVS == rive::ore::BindingMap::kAbsent &&
            *outFS == rive::ore::BindingMap::kAbsent)
        {
            setLastError("makeBindGroup: (group=%u, binding=%u) layout has "
                         "no resolved native slot — call "
                         "makeLayoutFromShader",
                         groupIndex,
                         binding);
            return false;
        }
        return true;
    };

    // UBO bindings.
    const uint32_t nBufs = std::min(desc.uboCount, 8u);
    bindGroup->m_ubos.reserve(nBufs);
    bindGroup->m_retainedBuffers.reserve(nBufs);
    for (uint32_t i = 0; i < nBufs; ++i)
    {
        const auto& entry = desc.ubos[i];
        rive::ore::OreBindGroupRHI::RHIUBOBinding binding{};
        auto* buffer = lite_rtti_cast<rive::ore::OreBufferRHI*>(entry.buffer);
        check(buffer);
        binding.buffer = buffer;
        binding.binding = entry.slot;
        if (!lookupStages(entry.slot,
                          rive::ore::BindingKind::uniformBuffer,
                          &binding.vsSlot,
                          &binding.fsSlot))
            continue;
        binding.offset = entry.offset;
        binding.size = (entry.size != 0)
                           ? entry.size
                           : (entry.buffer->size() - entry.offset);
        binding.hasDynamicOffset = layout->hasDynamicOffset(entry.slot);
        if (binding.hasDynamicOffset)
            bindGroup->m_dynamicOffsetCount++;
        bindGroup->m_ubos.push_back(binding);
        bindGroup->m_retainedBuffers.push_back(ref_rcp(entry.buffer));
    }
    // Sort UBOs by WGSL @binding so `dynamicOffsets[]` is consumed in
    // BindGroupLayout-entry order regardless of caller-supplied
    // `desc.ubos[]` ordering (WebGPU contract; matches Dawn).
    std::sort(bindGroup->m_ubos.begin(),
              bindGroup->m_ubos.end(),
              [](const rive::ore::OreBindGroupRHI::RHIUBOBinding& a,
                 const rive::ore::OreBindGroupRHI::RHIUBOBinding& b) {
                  return a.binding < b.binding;
              });

    // Texture bindings.
    const uint32_t nTexs = std::min(desc.textureCount, 8u);
    bindGroup->m_textures.reserve(nTexs);
    bindGroup->m_retainedViews.reserve(nTexs);
    for (uint32_t i = 0; i < nTexs; ++i)
    {
        const auto& entry = desc.textures[i];
        rive::ore::OreBindGroupRHI::RHITexBinding binding{};
        auto* view = lite_rtti_cast<rive::ore::OreTextureViewRHI*>(entry.view);
        check(view);
        binding.srv = view->m_srv;
        auto* tex = lite_rtti_cast<rive::ore::OreTextureRHI*>(view->texture());
        check(tex);
        binding.rhiTexture = tex->m_texture;
        if (!lookupStages(entry.slot,
                          rive::ore::BindingKind::sampledTexture,
                          &binding.vsSlot,
                          &binding.fsSlot))
            continue;
        bindGroup->m_textures.push_back(binding);
        bindGroup->m_retainedViews.push_back(ref_rcp(entry.view));
    }

    // Sampler bindings.
    const uint32_t nSamps = std::min(desc.samplerCount, 8u);
    bindGroup->m_samplers.reserve(nSamps);
    bindGroup->m_retainedSamplers.reserve(nSamps);
    for (uint32_t i = 0; i < nSamps; ++i)
    {
        const auto& entry = desc.samplers[i];
        rive::ore::OreBindGroupRHI::RHISamplerBinding binding{};
        auto* sampler =
            lite_rtti_cast<rive::ore::OreSamplerRHI*>(entry.sampler);
        check(sampler);
        binding.rhiSampler = sampler->m_sampler;
        if (!lookupStages(entry.slot,
                          rive::ore::BindingKind::sampler,
                          &binding.vsSlot,
                          &binding.fsSlot))
            continue;
        bindGroup->m_samplers.push_back(binding);
        bindGroup->m_retainedSamplers.push_back(ref_rcp(entry.sampler));
    }

    return bindGroup;
}

rive::rcp<rive::ore::Pipeline> OreContextRHI::makePipeline(
    const rive::ore::PipelineDesc& desc,
    std::string* outError)
{
    check(IsInRenderingThread());
    auto pipeline = rive::rcp<rive::ore::OrePipelineRHI>(
        new rive::ore::OrePipelineRHI(desc));

    {
        std::string err;
        if (!validateLayoutsAgainstBindingMap(pipeline->m_bindingMap,
                                              desc.bindGroupLayouts,
                                              desc.bindGroupLayoutCount,
                                              &err) ||
            !rive::ore::validateColorRequiresFragment(desc.colorCount,
                                                      desc.fragmentModule !=
                                                          nullptr,
                                                      &err))
        {
            if (outError)
                *outError = err;
            else
                setLastError("makePipeline: %s", err.c_str());
            return nullptr;
        }
    }

    FGraphicsPipelineStateInitializer& initializer =
        pipeline->m_pipelineStateInitializer;

    FRHIVertexShader* vertexShader = nullptr;
    FRHIPixelShader* pixelShader = nullptr;

    if (desc.vertexModule != nullptr)
    {
        // No need for dynamic cast because it will always be the rhi variant
        auto ShaderModule =
            static_cast<rive::ore::OreShaderModuleRHI*>(desc.vertexModule);
        vertexShader = ShaderModule->VertexShader;
        pipeline->m_vertexSlotRemap = ShaderModule->SlotRemap;
    }

    if (desc.fragmentModule != nullptr)
    {
        // No need for dynamic cast because it will always be the rhi variant
        auto ShaderModule =
            static_cast<rive::ore::OreShaderModuleRHI*>(desc.fragmentModule);
        pixelShader = ShaderModule->PixelShader;
        pipeline->m_fragmentSlotRemap = ShaderModule->SlotRemap;
    }

    if (vertexShader == nullptr)
    {
        if (outError != nullptr)
            *outError = "ORE RHI pipeline requires vertex shaders.";
        return nullptr;
    }

    FVertexDeclarationElementList vertexElements;

    for (uint32_t bufIdx = 0; bufIdx < desc.vertexBufferCount; ++bufIdx)
    {
        const auto& layout = desc.vertexBuffers[bufIdx];
        for (uint32_t ai = 0; ai < layout.attributeCount; ++ai)
        {
            assert(elementCount < std::size(elements));
            const auto& attr = layout.attributes[ai];
            FVertexElement Element;
            Element.AttributeIndex = attr.shaderSlot;
            Element.bUseInstanceIndex =
                layout.stepMode == rive::ore::VertexStepMode::instance;
            Element.StreamIndex = bufIdx;
            Element.Offset = attr.offset;
            Element.Stride = layout.stride;
            Element.Type = VertexFormatToVertexElement(attr.format);

            vertexElements.Add(MoveTemp(Element));
        }
    }

    // The initializer keeps only raw pointers; retain strong refs on the
    // pipeline so these outlive whatever created them (the shader modules, and
    // the transient RHICreate*State results) for the pipeline's whole lifetime.
    // Otherwise PSO creation AddRefs a freed resource and crashes.
    pipeline->m_vertexDeclaration = RHICreateVertexDeclaration(vertexElements);
    pipeline->m_vertexShader = vertexShader;
    pipeline->m_pixelShader = pixelShader;

    initializer.BoundShaderState.VertexDeclarationRHI =
        pipeline->m_vertexDeclaration;
    initializer.BoundShaderState.VertexShaderRHI = vertexShader;
    initializer.BoundShaderState.PixelShaderRHI = pixelShader;

    switch (desc.topology)
    {
        case rive::ore::PrimitiveTopology::triangleList:
            initializer.PrimitiveType = PT_TriangleList;
            break;
        case rive::ore::PrimitiveTopology::triangleStrip:
            initializer.PrimitiveType = PT_TriangleStrip;
            break;
        case rive::ore::PrimitiveTopology::lineList:
            initializer.PrimitiveType = PT_LineList;
            break;
        case rive::ore::PrimitiveTopology::pointList:
            initializer.PrimitiveType = PT_PointList;
            break;
        case rive::ore::PrimitiveTopology::lineStrip:
            if (outError)
                *outError = "ORE RHI pipeline does not support line strips.";
            return nullptr;
    }

    FRasterizerStateInitializerRHI rasterizerDesc;
    // Ore's depthBias is an integer in the D3D/WebGPU convention: a count of
    // minimum-resolvable depth units, applied directly as D3D12_RASTERIZER_DESC
    // ::DepthBias (an INT). UE instead takes a float DepthBias in normalized
    // [0,1] depth space and converts it back to the D3D12 integer via
    // FloorToInt(DepthBias * 2^24) (see FD3D12DynamicRHI). Passing the raw
    // integer would be scaled by 2^24 — e.g. a bias of 1 becomes 16M units,
    // pushing every shadow fragment past the far plane so it fails the depth
    // test and nothing is written. Divide by 2^24 so UE reconstructs exactly
    // the integer bias the other backends use.
    rasterizerDesc.DepthBias = static_cast<float>(desc.depthStencil.depthBias) /
                               static_cast<float>(1 << 24);
    rasterizerDesc.SlopeScaleDepthBias = desc.depthStencil.depthBiasSlopeScale;
    if (GRHIOreNeedsSlopeBiasAdjusted)
    {
        // Some RHIs scale the rasterizer's slope-scaled depth bias by the
        // depth- buffer resolution (2^24) before applying it, whereas
        // D3D/Vulkan/Metal use it as a raw multiplier of the depth slope. Left
        // unscaled, our slope bias (e.g. 1.5) becomes ~2^24x too large there,
        // pushing every sloped shadow fragment past the far plane so the depth
        // test fails and the shadow map is never written. Pre-divide by the
        // same factor so the platform reconstructs the intended bias. (UE's own
        // shadows avoid this because they apply depth bias in the shadow-depth
        // shader, not via the rasterizer state.) See OrePlatformRHI.h.
        rasterizerDesc.SlopeScaleDepthBias /= static_cast<float>(1 << 24);
    }

    // Yes, you would think rasterizerDesc.SlopeScaleDepthBias would act the
    // same as rasterizerDesc.DepthBias in terms of how it gets translated to a
    // backend. It does not.

    rasterizerDesc.DepthClipMode = ERasterizerDepthClipMode::DepthClip;
    rasterizerDesc.bAllowMSAA = !GForceOreSingleSample && desc.sampleCount > 1;
    rasterizerDesc.CullMode =
        ToRhiCullMode(desc.cullMode,
                      desc.winding == rive::ore::FaceWinding::counterClockwise);
    rasterizerDesc.FillMode = FM_Solid;

    pipeline->m_rasterizerState = RHICreateRasterizerState(rasterizerDesc);
    initializer.RasterizerState = pipeline->m_rasterizerState;

    static_assert(
        static_cast<EColorWriteMask>(rive::ore::ColorWriteMask::red) == CW_RED);
    static_assert(static_cast<EColorWriteMask>(
                      rive::ore::ColorWriteMask::green) == CW_GREEN);
    static_assert(static_cast<EColorWriteMask>(
                      rive::ore::ColorWriteMask::blue) == CW_BLUE);
    static_assert(static_cast<EColorWriteMask>(
                      rive::ore::ColorWriteMask::alpha) == CW_ALPHA);

    FBlendStateInitializerRHI blendDesc;
    blendDesc.bUseIndependentRenderTargetBlendStates = desc.colorCount > 1;
    blendDesc.bUseAlphaToCoverage = false;
    initializer.RenderTargetsEnabled = desc.colorCount;
    for (uint32 i = 0; i < desc.colorCount; i++)
    {
        initializer.RenderTargetFormats[i] =
            PixelFormatFromOreFormat(desc.colorTargets[i].format);
        initializer.RenderTargetFlags[i] = TexCreate_RenderTargetable;

        auto ColorTarget = desc.colorTargets[i];
        blendDesc.RenderTargets[i].ColorWriteMask =
            static_cast<EColorWriteMask>(ColorTarget.writeMask);
        if (ColorTarget.blendEnabled)
        {
            blendDesc.RenderTargets[i].ColorBlendOp =
                ToRhiBlendOp(ColorTarget.blend.colorOp);
            blendDesc.RenderTargets[i].AlphaBlendOp =
                ToRhiBlendOp(ColorTarget.blend.alphaOp);
            blendDesc.RenderTargets[i].ColorSrcBlend =
                ToRhiBlendFactor(ColorTarget.blend.srcColor);
            blendDesc.RenderTargets[i].ColorDestBlend =
                ToRhiBlendFactor(ColorTarget.blend.dstColor);
            blendDesc.RenderTargets[i].AlphaSrcBlend =
                ToRhiBlendFactor(ColorTarget.blend.srcAlpha);
            blendDesc.RenderTargets[i].AlphaDestBlend =
                ToRhiBlendFactor(ColorTarget.blend.dstAlpha);
        }
        else
        {
            blendDesc.RenderTargets[i].ColorSrcBlend = BF_One;
            blendDesc.RenderTargets[i].ColorDestBlend = BF_Zero;
            blendDesc.RenderTargets[i].AlphaSrcBlend = BF_One;
            blendDesc.RenderTargets[i].AlphaDestBlend = BF_Zero;
        }
    }

    pipeline->m_blendState = RHICreateBlendState(blendDesc);
    initializer.BlendState = pipeline->m_blendState;
    initializer.NumSamples = GForceOreSingleSample ? 1 : desc.sampleCount;

    FDepthStencilStateInitializerRHI DepthStencilStateInitializer;
    DepthStencilStateInitializer.DepthTest =
        ToRhiCompareFunction(desc.depthStencil.depthCompare);
    DepthStencilStateInitializer.bEnableDepthWrite =
        desc.depthStencil.depthWriteEnabled;

    const bool bHasFrontStencil =
        desc.stencilFront.compare != rive::ore::CompareFunction::always ||
        desc.stencilFront.failOp != rive::ore::StencilOp::keep ||
        desc.stencilFront.depthFailOp != rive::ore::StencilOp::keep ||
        desc.stencilFront.passOp != rive::ore::StencilOp::keep;

    const bool bHasBackStencil =
        desc.stencilBack.compare != rive::ore::CompareFunction::always ||
        desc.stencilBack.failOp != rive::ore::StencilOp::keep ||
        desc.stencilBack.depthFailOp != rive::ore::StencilOp::keep ||
        desc.stencilBack.passOp != rive::ore::StencilOp::keep;

    DepthStencilStateInitializer.bEnableFrontFaceStencil = bHasFrontStencil;
    DepthStencilStateInitializer.bEnableBackFaceStencil = bHasBackStencil;
    DepthStencilStateInitializer.StencilReadMask = desc.stencilReadMask;
    DepthStencilStateInitializer.StencilWriteMask = desc.stencilWriteMask;
    DepthStencilStateInitializer.FrontFaceStencilTest =
        ToRhiCompareFunction(desc.stencilFront.compare);
    DepthStencilStateInitializer.FrontFacePassStencilOp =
        ToRHIStencilOp(desc.stencilFront.passOp);
    DepthStencilStateInitializer.FrontFaceDepthFailStencilOp =
        ToRHIStencilOp(desc.stencilFront.depthFailOp);
    DepthStencilStateInitializer.FrontFaceStencilFailStencilOp =
        ToRHIStencilOp(desc.stencilFront.failOp);
    DepthStencilStateInitializer.BackFaceStencilTest =
        ToRhiCompareFunction(desc.stencilBack.compare);
    DepthStencilStateInitializer.BackFacePassStencilOp =
        ToRHIStencilOp(desc.stencilBack.passOp);
    DepthStencilStateInitializer.BackFaceDepthFailStencilOp =
        ToRHIStencilOp(desc.stencilBack.depthFailOp);
    DepthStencilStateInitializer.BackFaceStencilFailStencilOp =
        ToRHIStencilOp(desc.stencilBack.failOp);

    pipeline->m_depthStencilState =
        RHICreateDepthStencilState(DepthStencilStateInitializer);
    initializer.DepthStencilState = pipeline->m_depthStencilState;

    const FExclusiveDepthStencil::Type DepthOp =
        desc.depthStencil.depthWriteEnabled ? FExclusiveDepthStencil::DepthWrite
                                            : FExclusiveDepthStencil::DepthNop;
    const FExclusiveDepthStencil::Type StencilOp =
        bHasFrontStencil || bHasBackStencil
            ? FExclusiveDepthStencil::StencilWrite
            : FExclusiveDepthStencil::StencilNop;

    initializer.DepthStencilAccess =
        static_cast<FExclusiveDepthStencil::Type>(DepthOp + StencilOp);
    initializer.DepthStencilTargetFormat =
        PixelFormatFromOreFormat(desc.depthStencil.format);
    initializer.DepthStencilTargetFlag = TexCreate_DepthStencilTargetable;

    check(initializer.ComputeNumValidRenderTargets() == desc.colorCount);
    return pipeline;
}

std::unique_ptr<rive::ore::RenderPass> OreContextRHI::beginRenderPass(
    const rive::ore::RenderPassDesc& desc,
    std::string* outError)
{
    check(IsInRenderingThread());

    FRHICommandList& RHICmdList = getCommandList();

    auto renderPass =
        std::make_unique<rive::ore::OreRenderPassRHI>(this, RHICmdList);
    renderPass->populateAttachmentMetadata(desc);

    // Record the attachment dimensions so setViewport/setScissorRect can clamp
    // to them. The Ore renderer can hand us a viewport/scissor larger than the
    // target (notably a canvas-sized clip rect on the smaller shadow-map pass);
    // Metal asserts on an out-of-bounds scissor and drops the draw, blanking
    // the shadow map. Take the dimensions from the first color attachment, or
    // the depth target for a depth-only (shadow) pass.
    if (desc.colorCount >= 1 && desc.colorAttachments[0].view != nullptr)
    {
        renderPass->m_rtWidth =
            desc.colorAttachments[0].view->texture()->width();
        renderPass->m_rtHeight =
            desc.colorAttachments[0].view->texture()->height();
    }
    else if (desc.depthStencil.view != nullptr)
    {
        renderPass->m_rtWidth = desc.depthStencil.view->texture()->width();
        renderPass->m_rtHeight = desc.depthStencil.view->texture()->height();
    }

#if WITH_RHI_BREADCRUMBS
    // Name the pass so it's identifiable in a GPU capture. A depth-only pass
    // (colorCount == 0) is the shadow-map pass.
    const bool bDepthOnly =
        desc.colorCount == 0 && desc.depthStencil.view != nullptr;
    const FString PassName =
        FString::Printf(TEXT("Ore::%s [%hs] (color=%u%s)"),
                        bDepthOnly ? TEXT("ShadowPass") : TEXT("RenderPass"),
                        desc.label ? desc.label : "",
                        desc.colorCount,
                        desc.depthStencil.view ? TEXT(",depth") : TEXT(""));
    renderPass->m_breadcrumb.Emplace(
        RHICmdList,
        RHI_BREADCRUMB_DESC_COPY_VALUES(TEXT("OreRenderPass"),
                                        TEXT("%s"),
                                        RHI_GPU_STAT_ARGS_NONE)(PassName));
#endif

    FRHIRenderPassInfo PassInfo;
    // MultiViewCount enables stereo/array multiview — NOT the MRT count (that's
    // conveyed by the ColorRenderTargets array).
    PassInfo.MultiViewCount = 0;

    // BeginRenderPass already transitions the color/depth attachments it's told
    // about (and updates UE's state tracking accordingly), so we must NOT touch
    // those ourselves — doing so desyncs UE's tracking from the actual D3D12
    // state. The one thing it does NOT handle is the MSAA *resolve* target. At
    // EndRenderPass the resolve assumes the target is in RenderTarget state,
    // but ResolveSubresource needs ResolveDst. Transition only the resolve
    // target, before BeginRenderPass (RHI transitions are illegal inside a
    // pass). The resolve targets are wrapped external textures UE doesn't
    // track, so this can't desync it.
    TArray<FRHITransitionInfo, TInlineAllocator<8>> Transitions;

    if (desc.depthStencil.view && desc.colorCount == 0)
    {
        PassInfo.ResolveRect = {
            0,
            0,
            static_cast<signed int>(desc.depthStencil.view->texture()->width()),
            static_cast<signed int>(
                desc.depthStencil.view->texture()->height())};
    }
    else if (desc.colorCount >= 1 && desc.colorAttachments[0].resolveTarget)
    {
        PassInfo.ResolveRect = {
            0,
            0,
            static_cast<signed int>(
                desc.colorAttachments[0].resolveTarget->texture()->width()),
            static_cast<signed int>(
                desc.colorAttachments[0].resolveTarget->texture()->height())};
    }

    for (uint32 i = 0; i < desc.colorCount; ++i)
    {
        const auto& Attachment = desc.colorAttachments[i];
        ERenderTargetLoadAction LoadAction =
            RenderTargetLoadActionFromLoadOp(Attachment.loadOp);
        auto* tex = lite_rtti_cast<rive::ore::OreTextureRHI*>(
            Attachment.view->texture());
        auto* resolveTex = Attachment.resolveTarget
                               ? lite_rtti_cast<rive::ore::OreTextureRHI*>(
                                     Attachment.resolveTarget->texture())
                               : nullptr;

        // Single-sample debug path: skip MSAA entirely and render directly into
        // the (single-sample) resolve target that gets displayed/sampled, so
        // the MSAA source and the resolve pass are never used.
        const bool bRenderDirectToResolve =
            GForceOreSingleSample && resolveTex != nullptr;
        auto* renderTex = bRenderDirectToResolve ? resolveTex : tex;

        // Vulkan is the only platform where UE's render-pass auto-resolve does
        // not land in our resolve target: UE 5.9's dynamic-rendering
        // auto-resolve derives the resolve view from the multisample target, so
        // the resolve silently fails. There we run a manual averaging resolve
        // in finish(). Other backends use the native hardware resolve — the
        // canvas just has to be created so the resolve dest is valid (see
        // makeRenderCanvas).
        const bool bManualResolve = IsVulkanPlatform(GMaxRHIShaderPlatform);

        // EMultisampleResolve discards the MSAA samples (it expects UE to
        // resolve them). On the manual-resolve platforms we resolve ourselves
        // in finish(), so the MSAA must be STORED — otherwise the manual
        // resolve reads discarded garbage (random flickering geometry). Only
        // let UE auto-resolve (and discard) where its auto-resolve works.
        const bool bAutoResolve =
            resolveTex != nullptr && !bManualResolve && !bRenderDirectToResolve;
        ERenderTargetStoreAction StoreAction =
            RenderTargetStoreActionFromStoreOp(Attachment.storeOp,
                                               bAutoResolve);
        if (resolveTex != nullptr && !bAutoResolve)
        {
            StoreAction = ERenderTargetStoreAction::EStore;
        }

        // Move the color target into its attachment layout before the pass. A
        // prior pass or the manual MSAA resolve in finish() can leave a color
        // texture in a shader-read layout. UE's BeginRenderPass doesn't
        // transition these raw textures itself, so without this the attachment
        // is in the wrong Vulkan layout (VUID-09592) and rendering is wrong.
        Transitions.Add(
            FRHITransitionInfo(renderTex->m_texture, ERHIAccess::RTV));
        // Remember it so finish() can transition it back to a readable state
        // (write->read barrier for the next pass / the canvas composite).
        renderPass->m_colorTargetsToRead.Add(renderTex->m_texture);

        PassInfo.ColorRenderTargets[i].Action =
            MakeRenderTargetActions(LoadAction, StoreAction);
        PassInfo.ColorRenderTargets[i].RenderTarget = renderTex->m_texture;
        PassInfo.ColorRenderTargets[i].MipIndex =
            Attachment.view->baseMipLevel();
        PassInfo.ColorRenderTargets[i].ArraySlice =
            Attachment.view->baseLayer();

        if (resolveTex != nullptr && !bRenderDirectToResolve)
        {
            if (bManualResolve)
            {
                // Vulkan: don't ask UE to auto-resolve (broken — see
                // bManualResolve above); record the MSAA->canvas pair and run a
                // manual averaging resolve in finish() (see
                // OreMSAAResolve.usf).
                renderPass->m_pendingResolves.Add(
                    {tex->m_texture, resolveTex->m_texture});
            }
            else
            {
                PassInfo.ColorRenderTargets[i].ResolveTarget =
                    resolveTex->m_texture;
                Transitions.Add(FRHITransitionInfo(resolveTex->m_texture,
                                                   ERHIAccess::ResolveDst));
            }
        }
    }

    if (desc.depthStencil.view != nullptr)
    {
        auto* depthTex = lite_rtti_cast<rive::ore::OreTextureRHI*>(
            desc.depthStencil.view->texture());
        PassInfo.DepthStencilRenderTarget.DepthStencilTarget =
            depthTex->m_texture;
        // Only a depth-only pass (colorCount == 0) is a shadow map that a later
        // pass samples — those need the depth transitioned to a sampleable
        // state in finish() (and back to an attachment layout next frame). A
        // normal pass's depth is just for testing and is never sampled, so
        // leave its depth entirely to UE's BeginRenderPass; transitioning it
        // ourselves is unnecessary and the whole-resource barrier
        // (depth|stencil aspect) can disturb the depth test on Vulkan.
        const bool bDepthSampledLater = desc.colorCount == 0;
        if (bDepthSampledLater)
        {
            renderPass->m_depthTargetToSample = depthTex->m_texture;
        }

        ERenderTargetLoadAction DepthLoadAction =
            RenderTargetLoadActionFromLoadOp(desc.depthStencil.depthLoadOp);
        ERenderTargetStoreAction DepthStoreAction =
            RenderTargetStoreActionFromStoreOp(desc.depthStencil.depthStoreOp,
                                               false);
        // Only drive stencil load/store if the format actually has a stencil
        // aspect; otherwise UE warns ("clear a DSV that does not store
        // stencil") for depth-only targets (depth16unorm / depth32float).
        const rive::ore::TextureFormat DepthFormat =
            desc.depthStencil.view->texture()->format();
        const bool bHasStencil =
            DepthFormat == rive::ore::TextureFormat::depth24plusStencil8 ||
            DepthFormat == rive::ore::TextureFormat::depth32floatStencil8;
        ERenderTargetLoadAction StencilLoadAction =
            bHasStencil ? RenderTargetLoadActionFromLoadOp(
                              desc.depthStencil.stencilLoadOp)
                        : ERenderTargetLoadAction::ENoAction;
        ERenderTargetStoreAction StencilStoreAction =
            bHasStencil ? RenderTargetStoreActionFromStoreOp(
                              desc.depthStencil.stencilStoreOp,
                              false)
                        : ERenderTargetStoreAction::ENoAction;
        PassInfo.DepthStencilRenderTarget.Action =
            MakeDepthStencilTargetActions(
                MakeRenderTargetActions(DepthLoadAction, DepthStoreAction),
                MakeRenderTargetActions(StencilLoadAction, StencilStoreAction));

        // The depth-stencil view must be bound *writable* when the pass clears
        // or writes depth/stencil. Without this the DSV defaults to read-only,
        // so clearing/writing it fails silently on D3D12 (the shadow depth
        // pass produces nothing -> no shadows), and as an ensure (assert) on
        // D3D11.
        const bool bDepthWrite =
            DepthLoadAction == ERenderTargetLoadAction::EClear ||
            DepthStoreAction == ERenderTargetStoreAction::EStore;
        const bool bStencilWrite =
            bHasStencil &&
            (StencilLoadAction == ERenderTargetLoadAction::EClear ||
             StencilStoreAction == ERenderTargetStoreAction::EStore);
        const FExclusiveDepthStencil::Type DepthAccess =
            bDepthWrite ? FExclusiveDepthStencil::DepthWrite
                        : FExclusiveDepthStencil::DepthRead;
        const FExclusiveDepthStencil::Type StencilAccess =
            bHasStencil ? (bStencilWrite ? FExclusiveDepthStencil::StencilWrite
                                         : FExclusiveDepthStencil::StencilRead)
                        : FExclusiveDepthStencil::StencilNop;
        PassInfo.DepthStencilRenderTarget.ExclusiveDepthStencil =
            FExclusiveDepthStencil(static_cast<FExclusiveDepthStencil::Type>(
                DepthAccess + StencilAccess));

        if (bDepthSampledLater)
        {
            const ERHIAccess DepthRHIAccess =
                bDepthWrite ? ERHIAccess::DSVWrite : ERHIAccess::DSVRead;
            Transitions.Add(
                FRHITransitionInfo(depthTex->m_texture, DepthRHIAccess));
        }
    }

    if (Transitions.Num() > 0)
    {
        RHICmdList.Transition(MakeArrayView(Transitions));
    }

    RHICmdList.BeginRenderPass(PassInfo, TEXT("RiveOreRenderPass"));

    // Reset viewport and scissor to the full attachment bounds. UE's RHI keeps
    // scissor state across render passes, so a canvas-sized scissor set in a
    // previous (larger) pass persists into a smaller one (e.g. the shadow-map
    // pass). The smaller pass may draw without setting its own scissor first,
    // and Metal validates the stale, out-of-bounds scissor against the new
    // render target at draw time and asserts. the draw is dropped and the
    // shadow map is blank. The Ore renderer overrides these per-draw where it
    // needs a tighter clip.
    if (renderPass->m_rtWidth > 0 && renderPass->m_rtHeight > 0)
    {
        RHICmdList.SetViewport(0.0f,
                               0.0f,
                               0.0f,
                               static_cast<float>(renderPass->m_rtWidth),
                               static_cast<float>(renderPass->m_rtHeight),
                               1.0f);
        RHICmdList.SetScissorRect(true,
                                  0,
                                  0,
                                  renderPass->m_rtWidth,
                                  renderPass->m_rtHeight);
    }

    return renderPass;
}

void OreContextRHI::beginFrame(const rive::ore::Context::FrameDescriptor&) {}
void OreContextRHI::endFrame() {}

void OreContextRHI::waitForGPU() {}

rive::rcp<rive::ore::TextureView> OreContextRHI::wrapCanvasTexture(
    rive::gpu::RenderCanvas* canvas)
{
    check(canvas != nullptr);

    auto* Target = static_cast<RenderTargetRHI*>(canvas->renderTarget());
    auto RHITexture = Target->texture();

    rive::ore::TextureDesc TextureDesc;
    TextureDesc.width = RHITexture->GetSizeX();
    TextureDesc.height = RHITexture->GetSizeY();
    TextureDesc.format = OreFormatFromPixelFormat(RHITexture->GetFormat());
    TextureDesc.depthOrArrayLayers = RHITexture->GetSizeZ();
    TextureDesc.numMipmaps = RHITexture->GetNumMips();
    TextureDesc.sampleCount = RHITexture->GetNumSamples();
    TextureDesc.type =
        OreTextureTypeFromDimension(RHITexture->GetDesc().Dimension);
    auto Texture =
        rive::make_rcp<rive::ore::OreTextureRHI>(MoveTemp(TextureDesc));
    Texture->m_texture = RHITexture;

    rive::ore::TextureViewDesc ViewDesc;
    ViewDesc.texture = Texture.get();
    ViewDesc.baseLayer = 0;
    ViewDesc.baseMipLevel = 0;
    ViewDesc.layerCount = 1;

    return rive::make_rcp<rive::ore::OreTextureViewRHI>(MoveTemp(Texture),
                                                        MoveTemp(ViewDesc));
}

rive::rcp<rive::ore::TextureView> OreContextRHI::wrapRiveTexture(
    rive::gpu::Texture* gpuTex,
    uint32_t width,
    uint32_t height)
{
    check(gpuTex != nullptr);
    auto* GPUTexture = static_cast<TextureRHIImpl*>(gpuTex);
    auto RHITexture = GPUTexture->contents();

    rive::ore::TextureDesc TextureDesc;
    TextureDesc.width = RHITexture->GetSizeX();
    TextureDesc.height = RHITexture->GetSizeY();
    TextureDesc.format = OreFormatFromPixelFormat(RHITexture->GetFormat());
    TextureDesc.depthOrArrayLayers = RHITexture->GetSizeZ();
    TextureDesc.numMipmaps = RHITexture->GetNumMips();
    TextureDesc.sampleCount = RHITexture->GetNumSamples();
    TextureDesc.type =
        OreTextureTypeFromDimension(RHITexture->GetDesc().Dimension);
    auto Texture =
        rive::make_rcp<rive::ore::OreTextureRHI>(MoveTemp(TextureDesc));
    Texture->m_texture = RHITexture;

    rive::ore::TextureViewDesc ViewDesc;
    ViewDesc.texture = Texture.get();
    ViewDesc.baseLayer = 0;
    ViewDesc.baseMipLevel = 0;
    ViewDesc.layerCount = 1;

    return rive::make_rcp<rive::ore::OreTextureViewRHI>(MoveTemp(Texture),
                                                        MoveTemp(ViewDesc));
}
