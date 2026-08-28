// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveShaderTypes.h"

#include <filesystem>

#include <CoreMinimal.h>
#include "GlobalShader.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "Misc/ConfigCacheIni.h"
#include "ShaderPlatformCachedIniValue.h"
#include "ShaderCompilerCore.h"
#include "Interfaces/IPluginManager.h"

THIRD_PARTY_INCLUDES_START
#include "rive/generated/shaders/rhi.glsl.exports.h"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY(LogRiveShaderCompiler);

// The ini value the msaa shaders were compiled against. Read from config
// rather than the cvar: for the running platform the cached ini value returns
// the live cvar, which a -dpcvars override has already changed.
bool RiveCookedReadAttachmentInPlace()
{
    static const bool bCooked = [] {
        bool bValue = false;
        GConfig->GetBool(TEXT("SystemSettings"),
                         TEXT("r.rive.ReadAttachmentInPlace"),
                         bValue,
                         GEngineIni);
        return bValue;
    }();
    return bCooked;
}

// Reads r.rive.ReadAttachmentInPlace as configured for the platform being
// cooked for. Those platforms fetch the live 4x target per sample.
static bool RivePlatformReadsAttachmentInPlace(
    const FShaderPermutationParameters& Params)
{
    static FShaderPlatformCachedIniValue<bool> ReadInPlaceIniValue(
        TEXT("r.rive.ReadAttachmentInPlace"));
    return ReadInPlaceIniValue.Get(Params.Platform);
}

bool RivePlatformSupportsSubpassLoad(const FShaderPermutationParameters& Params)
{
    bool bSupported = IsTargetMetal(Params);
#if defined(UE_RHI_HAS_FRAMEBUFFER_FETCH_SUBPASS)
    bSupported = bSupported || IsTargetVulkan(Params);
#endif
    return bSupported;
}

void ModifyShaderEnvironment(const FShaderPermutationParameters& Params,
                             FShaderCompilerEnvironment& Environment,
                             const bool IsVertexShader,
                             const bool bWantsSubpassLoad)
{
#if UE_VERSION_OLDER_THAN(5, 5, 0) || RIVE_FORCE_USE_GENERATED_UNIFORMS
    Environment.SetDefine(TEXT("UNIFORM_DEFINITIONS_AUTO_GENERATED"),
                          TEXT("1"));
#endif

    Environment.SetDefine(TEXT("FORCE_ATOMIC_BUFFER"), Params.Platform == 39);

    if (Params.Platform == 39 || Params.Platform == 43)
    {
        Environment.SetDefine(TEXT("NEEDS_PATH_ID_CLAMP_WORKAROUND"), 1);
    }

    // Making this a permutation causes us to exceed the permutation limit in
    // unreal. For now, since no platforms we target need to be packed. We can
    // just leave it on. If we need to revisit this later we can.
    Environment.SetDefine(TEXT("ENABLE_TYPED_UAV_LOAD_STORE"), TEXT("1"));

    if (Params.Platform != 33)
    {
        Environment.SetDefine(TEXT("NEEDS_USHORT_DEFINE"), TEXT("1"));
    }

    if (IsVertexShader)
    {
        Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
    }
    else
    {
        Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
        Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
        // Only set when engine patch is present.
        if (bWantsSubpassLoad && RivePlatformSupportsSubpassLoad(Params))
        {
            Environment.SetDefine(TEXT("SUPPORTS_SUBPASS_LOAD"), TEXT("1"));
        }
        else if (RivePlatformReadsAttachmentInPlace(Params))
        {
            Environment.SetDefine(TEXT("SUPPORTS_MSAA_DST_TEXEL_FETCH"),
                                  TEXT("1"));
        }
    }
// 5.4 and up
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4 && !PLATFORM_APPLE
    // We are not bindless so this flag must be added for vulkan to work
    Environment.CompilerFlags.Add(CFLAG_ForceBindful);
#endif
}

void FRiveBasePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false, false);
}

void FRiveBaseVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true, false);
}

void FRiveRDGTessPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    FRiveBasePixelShader::ModifyCompilationEnvironment(Params, Environment);
    auto& Value = Environment.RenderTargetOutputFormatsMap.FindOrAdd(0);
    Value = static_cast<uint8>(EPixelFormat::PF_R32G32B32A32_UINT);
}

void FRiveRDGDrawAtlasFillPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    FRiveBasePixelShader::ModifyCompilationEnvironment(Params, Environment);
    auto& Value = Environment.RenderTargetOutputFormatsMap.FindOrAdd(0);
    Value = static_cast<uint8>(EPixelFormat::PF_R16F);
}

void FRiveRDGDrawAtlasStrokePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    FRiveBasePixelShader::ModifyCompilationEnvironment(Params, Environment);
    auto& Value = Environment.RenderTargetOutputFormatsMap.FindOrAdd(0);
    Value = static_cast<uint8>(EPixelFormat::PF_R16F);
}

void FRiveBltTextureAsDrawVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true, false);
}

void ModifyMSAASubpassShaderEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false, true);
}

void ModifyMSAAPathVertexShaderEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true, false);

    // Vulkan's SV_InstanceID already includes the draw's first instance, so
    // the baseInstance uniform is compiled out.
    if (IsTargetVulkan(Params))
    {
        Environment.SetDefine(TEXT("SV_INSTANCE_ID_INCLUDES_BASE"), TEXT("1"));
    }

#if defined(UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE)
    if (IsTargetVulkan(Params))
    {
        Environment.SetDefine(TEXT("EMULATE_DYNAMIC_COLOR_WRITE_DISABLE"),
                              TEXT("1"));
    }
#endif
}

IMPLEMENT_GLOBAL_SHADER(FRiveRDGGradientPixelShader,
                        "/Plugin/Rive/Private/Rive/color_ramp.usf",
                        GLSL_colorRampFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveRDGGradientVertexShader,
                        "/Plugin/Rive/Private/Rive/color_ramp.usf",
                        GLSL_colorRampVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGTessPixelShader,
                        "/Plugin/Rive/Private/Rive/tessellate.usf",
                        GLSL_tessellateFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveRDGTessVertexShader,
                        "/Plugin/Rive/Private/Rive/tessellate.usf",
                        GLSL_tessellateVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGPathPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveABRDGPathPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGPathVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_path.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGInteriorTrianglesPixelShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveABRDGInteriorTrianglesPixelShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGInteriorTrianglesVertexShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtlasBlitPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_atlas_blit.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveABRDGAtlasBlitPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_atlas_blit.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtlasBlitVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_atlas_blit.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageRectPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveABRDGImageRectPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageRectVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageMeshPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_mesh.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveABRDGImageMeshPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_mesh.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageMeshVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_mesh.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtomicResolvePixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveABRDGAtomicResolvePixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtomicResolveVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderPathPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderPathVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_path.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderInteriorTrianglesPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_interior_triangles.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderInteriorTrianglesVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_interior_triangles.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderImageMeshPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_image_mesh.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderImageMeshVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_image_mesh.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderAtlasBlitPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_blit.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGRasterOrderAtlasBlitVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_blit.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGAtlasBlitMSAAPixelShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_atlas_blit.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGAtlasBlitMSAAVertexShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_atlas_blit.usf",
    GLSL_drawVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGImageMeshMSAAPixelShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_image_mesh.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGImageMeshMSAAVertexShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_image_mesh.usf",
    GLSL_drawVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGPathMSAAPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_depthstencil_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGPathMSAAVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_depthstencil_path.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGStencilMSAAPixelShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_triangles_nocolor.usf",
    GLSL_blitFragmentMain,
    SF_Pixel);

// Same entry points as the shaders above, compiled with SUPPORTS_SUBPASS_LOAD.
IMPLEMENT_GLOBAL_SHADER(FRiveRDGPathMSAASubpassPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_depthstencil_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGAtlasBlitMSAASubpassPixelShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_atlas_blit.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGImageMeshMSAASubpassPixelShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_image_mesh.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(
    FRiveRDGStencilMSAAVertexShader,
    "/Plugin/Rive/Private/Rive/draw_depthstencil_triangles_nocolor.usf",
    GLSL_stencilVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGDrawAtlasFillPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_fill.usf",
                        GLSL_atlasFillFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGDrawAtlasStrokePixelShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_stroke.usf",
                        GLSL_atlasStrokeFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveBltTextureAsDrawVertexShader,
                        "/Plugin/Rive/Private/Rive/blend_texture.usf",
                        GLSL_blitVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveBltTextureAsDrawPixelShader,
                        "/Plugin/Rive/Private/Rive/blend_texture.usf",
                        GLSL_blitFragmentMain,
                        SF_Pixel);

// this could be either draw_atlas usf file,
IMPLEMENT_GLOBAL_SHADER(FRiveRDGDrawAtlasVertexShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_fill.usf",
                        GLSL_atlasVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGBltU32AsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_u32_as_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGBltR16FAsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_f16_as_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGBltU324AsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_u324_to_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGVisualizeBufferPixelShader,
                        "/Plugin/Rive/Private/Rive/visualize_buffer.usf",
                        "FragmentMain",
                        SF_Pixel);

#if UE_VERSION_OLDER_THAN(5, 5, 0) || RIVE_FORCE_USE_GENERATED_UNIFORMS
IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(FlushUniformSlot);
IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FFlushUniforms,
                                       "uniforms",
                                       FlushUniformSlot);
#else
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFlushUniforms, GLSL_FlushUniforms);
#endif

void BindStaticFlushUniforms(FRHICommandList& RHICmdList,
                             FUniformBufferRHIRef FlushUniforms)
{
    FUniformBufferStaticBindings Bindings;
    Bindings.AddUniformBuffer(FlushUniforms);
    RHICmdList.SetStaticUniformBuffers(Bindings);
}
