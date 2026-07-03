/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RenderGraphBuilder.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_context.hpp"
THIRD_PARTY_INCLUDES_END

class OreContextRHI : public rive::ore::Context
{
public:
    static std::unique_ptr<rive::ore::Context> make()
    {
        return std::unique_ptr<rive::ore::Context>(new OreContextRHI);
    }

    virtual rive::rcp<rive::ore::Buffer> makeBuffer(
        const rive::ore::BufferDesc& desc) override;
    virtual rive::rcp<rive::ore::Texture> makeTexture(
        const rive::ore::TextureDesc& desc) override;
    virtual rive::rcp<rive::ore::TextureView> makeTextureView(
        const rive::ore::TextureViewDesc& desc) override;
    virtual rive::rcp<rive::ore::Sampler> makeSampler(
        const rive::ore::SamplerDesc& desc) override;
    virtual rive::rcp<rive::ore::ShaderModule> makeShaderModule(
        const rive::ore::ShaderModuleDesc& desc) override;
    virtual rive::rcp<rive::ore::BindGroupLayout> makeBindGroupLayout(
        const rive::ore::BindGroupLayoutDesc& desc) override;
    virtual rive::rcp<rive::ore::Pipeline> makePipeline(
        const rive::ore::PipelineDesc& desc,
        std::string* outError = nullptr) override;
    virtual rive::rcp<rive::ore::BindGroup> makeBindGroup(
        const rive::ore::BindGroupDesc& desc) override;

    virtual std::unique_ptr<rive::ore::RenderPass> beginRenderPass(
        const rive::ore::RenderPassDesc& desc,
        std::string* outError = nullptr) override;

    virtual void beginFrame(
        const rive::ore::Context::FrameDescriptor&) override;
    virtual void endFrame() override;

    virtual void waitForGPU() override;

    virtual rive::rcp<rive::ore::TextureView> wrapCanvasTexture(
        rive::gpu::RenderCanvas* canvas) override;
    virtual rive::rcp<rive::ore::TextureView> wrapRiveTexture(
        rive::gpu::Texture* gpuTex,
        uint32_t width,
        uint32_t height) override;

    virtual rive::ore::ShaderTarget shaderTarget() const override
    {
        return rive::ore::ShaderTarget::hlsl;
    }

    // Injected by the caller (SRiveLeafWidget) for the duration of an Ore
    // canvas render. The Ore canvas is drawn inside an RDG pass lambda so its
    // raw-RHI work is bracketed by the render graph; we use the command list
    // RDG hands that pass rather than grabbing the immediate list directly
    // (which, under a live FRDGBuilder, desyncs RDG's state tracking). Cleared
    // back to null when the pass finishes.
    void setCurrentCommandList(FRHICommandList* CmdList)
    {
        m_currentCommandList = CmdList;
    }

    // Defined in the .cpp so it can query the RHI capability globals (which
    // need RHI headers) to populate the Ore Features struct.
    OreContextRHI();

private:
    // Returns the command list to record Ore work onto: the RDG-pass list while
    // a canvas render is in flight, otherwise the immediate list (resource
    // creation outside a render keeps its previous behavior).
    FRHICommandList& getCommandList() const
    {
        return m_currentCommandList ? *m_currentCommandList
                                    : GRHICommandList.GetImmediateCommandList();
    }

    FRHICommandList* m_currentCommandList = nullptr;
};
