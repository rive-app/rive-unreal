/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RenderGraphBuilder.h"
#include "RHIBreadcrumbs.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_render_pass.hpp"
#include "rive/renderer/ore/ore_buffer.hpp"
THIRD_PARTY_INCLUDES_END

#include "Ore/ore_bind_group_rhi.hpp"
#include "Ore/OreSlotRemap.h"

// Debug toggle: force the Ore backend to single-sample (no MSAA, no resolve) to
// isolate MSAA/resolve-specific issues from the rest of the draw path. When
// true, color attachments that have a resolve target render directly into that
// (single-sample) target and the MSAA + manual-resolve path is bypassed.
static constexpr bool GForceOreSingleSample = false;

namespace rive::ore
{

class OreRenderPassRHI : public RenderPass
{
public:
    OreRenderPassRHI(Context* context, FRHICommandList& RHICmdList);
    virtual ~OreRenderPassRHI() = default;

    virtual void setPipeline(Pipeline* pipeline) override;

    virtual void setVertexBuffer(uint32_t slot,
                                 Buffer* buffer,
                                 uint32_t offset = 0) override;
    virtual void setIndexBuffer(Buffer* buffer,
                                IndexFormat format,
                                uint32_t offset = 0) override;

    virtual void setBindGroup(uint32_t groupIndex,
                              BindGroup* bg,
                              const uint32_t* dynamicOffsets = nullptr,
                              uint32_t dynamicOffsetCount = 0) override;

    virtual void setViewport(float x,
                             float y,
                             float width,
                             float height,
                             float minDepth = 0.0f,
                             float maxDepth = 1.0f) override;
    virtual void setScissorRect(uint32_t x,
                                uint32_t y,
                                uint32_t width,
                                uint32_t height) override;

    virtual void setStencilReference(uint32_t ref) override;
    virtual void setBlendColor(float r, float g, float b, float a) override;

    virtual void draw(uint32_t vertexCount,
                      uint32_t instanceCount = 1,
                      uint32_t firstVertex = 0,
                      uint32_t firstInstance = 0) override;
    virtual void drawIndexed(uint32_t indexCount,
                             uint32_t instanceCount = 1,
                             uint32_t firstIndex = 0,
                             int32_t baseVertex = 0,
                             uint32_t firstInstance = 0) override;

    virtual void finish() override;

    // Set by OreContextRHI::beginRenderPass to the depth target, if any. After
    // the pass ends it's transitioned to a sampleable state so a later pass can
    // read it as a shadow map (raw RHI textures aren't auto-transitioned).
    FRHITexture* m_depthTargetToSample = nullptr;

    // Dimensions of this pass's attachments, set by beginRenderPass. The Ore
    // renderer can hand us a viewport/scissor larger than the target (e.g. the
    // canvas-sized clip rect on the smaller shadow-map pass); D3D/Vulkan clamp
    // such a rect silently, but Metal's validation asserts and the draw is lost
    // -> a blank shadow map. setViewport/setScissorRect clamp to these.
    uint32 m_rtWidth = 0;
    uint32 m_rtHeight = 0;

    // Vulkan-only: MSAA color targets that need a manual resolve after the pass
    // (UE 5.9's Vulkan dynamic-rendering auto-resolve is broken). Each entry is
    // (multisample source, single-sample destination); finish() runs a
    // fullscreen averaging resolve for each. Empty on D3D/Metal (auto-resolve).
    struct FPendingResolve
    {
        FRHITexture* Source = nullptr; // MSAA color target
        FRHITexture* Dest = nullptr;   // single-sample resolve target
    };
    TArray<FPendingResolve, TInlineAllocator<4>> m_pendingResolves;

    // Color targets this pass rendered into. finish() transitions them to a
    // shader-read state.
    TArray<FRHITexture*, TInlineAllocator<4>> m_colorTargetsToRead;

#if WITH_RHI_BREADCRUMBS
    // GPU debug marker spanning the whole pass (begun in beginRenderPass, ended
    // in finish()), so each Ore pass — and especially the depth-only shadow
    // pass — shows up as a named region in RenderDoc / PIX captures.
    TOptional<FRHIBreadcrumbEventManual> m_breadcrumb;
#endif

private:
    uint32 GetPrimitiveCount(uint32 vertexCount) const;

    // Snapshots the UBO's bound slice (static offset + resolved dynamic
    // offset) out of the buffer's CPU shadow into a single-draw uniform
    // buffer. UE has no offset-carrying uniform-buffer bind, so each
    // setBindGroup re-creates the slice. Single-draw uniform buffers are
    // pooled by the RHI, so this is the intended per-draw path.
    FUniformBufferRHIRef ResolveUBO(const OreBindGroupRHI::RHIUBOBinding& ubo,
                                    uint32_t dynamicOffset);

    FRHICommandList& RHICmdList;
    rcp<Buffer> m_indexBuffer;
    IndexFormat m_indexFormat = {};
    uint32_t m_indexOffset = 0;

    // Used to calculate the number of primites to pass to draw commands.
    EPrimitiveType m_primitiveType;

    // Per-stage Vulkan slot remaps of the currently bound pipeline (point into
    // the OrePipelineRHI; null until setPipeline). Empty on D3D/Metal, so
    // setBindGroup passes slots straight through there.
    const FOreVulkanSlotRemap* m_vsRemap = nullptr;
    const FOreVulkanSlotRemap* m_fsRemap = nullptr;

    // Hold strong references so bound groups (and the resources they
    // retain) stay alive until finish() — matches the D3D11 backend.
    rcp<BindGroup> m_boundGroups[kMaxBindGroups];
};

} // namespace rive::ore
