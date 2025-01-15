#include "RiveShaderTypes.h"

#include <filesystem>

#include <CoreMinimal.h>
#include "GlobalShader.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"

THIRD_PARTY_INCLUDES_START
#include "rive/generated/shaders/rhi.glsl.hpp"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY(LogRiveShaderCompiler);

void FRiveRDGGradientPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
}

void FRiveRDGGradientVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGTessPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
}

void FRiveRDGTessVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGPathPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
}

void FRiveRDGPathVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGInteriorTrianglesPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
}

void FRiveRDGInteriorTrianglesVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGImageRectPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
}

void FRiveRDGImageRectVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGImageMeshPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
}

void FRiveRDGImageMeshVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveRDGAtomicResolvePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
}

void FRiveRDGAtomicResolveVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
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

IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(FlushUniformSlot);
IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FFlushUniforms,
                                       "uniforms",
                                       FlushUniformSlot);

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FImageDrawUniforms,
                                         "imageDrawUniforms");

void BindStaticFlushUniforms(FRHICommandList& RHICmdList,
                             FUniformBufferRHIRef FlushUniforms)
{
    FUniformBufferStaticBindings Bindings;
    Bindings.AddUniformBuffer(FlushUniforms);
    RHICmdList.SetStaticUniformBuffers(Bindings);
}