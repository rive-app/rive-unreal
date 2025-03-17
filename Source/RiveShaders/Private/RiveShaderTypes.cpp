#include "RiveShaderTypes.h"

#include <filesystem>

#include <CoreMinimal.h>
#include "GlobalShader.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"
#include "Interfaces/IPluginManager.h"

THIRD_PARTY_INCLUDES_START
#include "rive/generated/shaders/rhi.glsl.hpp"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY(LogRiveShaderCompiler);

void ModifyShaderEnvironment(const FShaderPermutationParameters& Params,
                             FShaderCompilerEnvironment& Environment,
                             const bool IsVertexShader)
{

#if UE_VERSION_OLDER_THAN(5, 5, 0)
    Environment.SetDefine(TEXT("UNIFORM_DEFINITIONS_AUTO_GENERATED"),
                          TEXT("1"));
#endif

    Environment.SetDefine(TEXT("NEEDS_USHORT_DEFINE"), TEXT("1"));

    if (IsVertexShader)
    {
        Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
    }
    else
    {
        Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
        Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
    }
// 5.4 and up
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    // We are not bindless so this flag must be added for vulkan to work
    Environment.CompilerFlags.Add(CFLAG_ForceBindful);
#endif
}

void FRiveRDGGradientPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGGradientVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGTessPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
    Environment.SetRenderTargetOutputFormat(0, PF_R32G32B32A32_UINT);
}

void FRiveRDGTessVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGPathPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGPathVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGInteriorTrianglesPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGInteriorTrianglesVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGAtlasBlitPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGAtlasBlitVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGImageRectPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGImageRectVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGImageMeshPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGImageMeshVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGAtomicResolvePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
}

void FRiveRDGAtomicResolveVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
}

void FRiveRDGDrawAtlasFillPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
    Environment.SetRenderTargetOutputFormat(0, PF_R32_FLOAT);
}

void FRiveRDGDrawAtlasStrokePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, false);
    Environment.SetRenderTargetOutputFormat(0, PF_R32_FLOAT);
}

void FRiveRDGDrawAtlasVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    ModifyShaderEnvironment(Params, Environment, true);
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
    FRiveRDGInteriorTrianglesVertexShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtlasBlitPixelShader,
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

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageRectVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGImageMeshPixelShader,
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

IMPLEMENT_GLOBAL_SHADER(FRiveRDGAtomicResolveVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGDrawAtlasFillPixelShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_fill.usf",
                        GLSL_atlasFillFragmentMain,
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGDrawAtlasStrokePixelShader,
                        "/Plugin/Rive/Private/Rive/draw_atlas_stroke.usf",
                        GLSL_atlasStrokeFragmentMain,
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

IMPLEMENT_GLOBAL_SHADER(FRiveRDGBltU324AsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_u324_to_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveRDGVisualizeBufferPixelShader,
                        "/Plugin/Rive/Private/Rive/visualize_buffer.usf",
                        "FragmentMain",
                        SF_Pixel);

#if UE_VERSION_OLDER_THAN(5, 5, 0)
IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(FlushUniformSlot);
IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FFlushUniforms,
                                       "uniforms",
                                       FlushUniformSlot);
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FImageDrawUniforms, "imageDrawUniforms");
#else
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFlushUniforms, GLSL_FlushUniforms);
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FImageDrawUniforms, GLSL_ImageDrawUniforms);
#endif

void BindStaticFlushUniforms(FRHICommandList& RHICmdList,
                             FUniformBufferRHIRef FlushUniforms)
{
    FUniformBufferStaticBindings Bindings;
    Bindings.AddUniformBuffer(FlushUniforms);
    RHICmdList.SetStaticUniformBuffers(Bindings);
}