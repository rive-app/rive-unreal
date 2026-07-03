/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RHI.h"
#include "RHIResources.h"
#include "PipelineStateCache.h"
#include "Ore/OreSlotRemap.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_pipeline.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::ore
{

class OrePipelineRHI : public LITE_RTTI_OVERRIDE(Pipeline, OrePipelineRHI)
{
public:
    OrePipelineRHI(const PipelineDesc& desc);
    virtual ~OrePipelineRHI() = default;

    FGraphicsPipelineStateInitializer m_pipelineStateInitializer;

    // Strong references to everything m_pipelineStateInitializer only holds as
    // raw pointers. RHICreate*State / the shader modules return ref-counted
    // objects. Without retaining them here they can be freed before this
    // pipeline's PSO is compiled, and SetGraphicsPipelineState then AddRefs a
    // dangling resource and crashes (notably RasterizerState under PIE).
    FVertexShaderRHIRef m_vertexShader;
    FPixelShaderRHIRef m_pixelShader;
    FVertexDeclarationRHIRef m_vertexDeclaration;
    FRasterizerStateRHIRef m_rasterizerState;
    FBlendStateRHIRef m_blendState;
    FDepthStencilStateRHIRef m_depthStencilState;

    // Per-stage Vulkan register->bind-index remaps, copied from the shader
    // modules at makePipeline. Empty on D3D/Metal. Consumed by
    // OreRenderPassRHI::setBindGroup to translate slots before binding.
    FOreVulkanSlotRemap m_vertexSlotRemap;
    FOreVulkanSlotRemap m_fragmentSlotRemap;
};

} // namespace rive::ore
