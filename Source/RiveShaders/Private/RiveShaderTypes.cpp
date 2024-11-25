#include "RiveShaderTypes.h"

#include <filesystem>

#include <CoreMinimal.h>
#include "GlobalShader.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"

THIRD_PARTY_INCLUDES_START
#include "rive/generated/shaders/constants.glsl.hpp"
THIRD_PARTY_INCLUDES_END

void CheckTypedUAVs(const FShaderPermutationParameters& Params,
                    FShaderCompilerEnvironment& Environment)
{
    // We can't use RHICapabilities.bSupportsTypedUAVLoads here because the
    // platform we are checking against is the currently compiled for platform.
    // Which may not be the maximum possible platform which is what
    // RHICapabilities checks against
    if (RHISupports4ComponentUAVReadWrite(Params.Platform))
    {
        Environment.SetDefine(TEXT(GLSL_ENABLE_TYPED_UAV_LOAD_STORE), 1);
        Environment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
    }
}

void FRiveGradientPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
}

void FRiveGradientVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveTessPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
}

void FRiveTessVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRivePathPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    CheckTypedUAVs(Params, Environment);
}

void FRivePathVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveInteriorTrianglesPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    CheckTypedUAVs(Params, Environment);
}

void FRiveInteriorTrianglesVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveImageRectPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    CheckTypedUAVs(Params, Environment);
}

void FRiveImageRectVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveImageMeshPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    CheckTypedUAVs(Params, Environment);
}

void FRiveImageMeshVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveAtomicResolvePixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("FRAGMENT"), TEXT("1"));
    CheckTypedUAVs(Params, Environment);
}

void FRiveAtomicResolveVertexShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{
    Environment.SetDefine(TEXT("VERTEX"), TEXT("1"));
}

void FRiveBltU32AsF4PixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{}

void FRiveVisualizeBufferPixelShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters& Params,
    FShaderCompilerEnvironment& Environment)
{}

IMPLEMENT_GLOBAL_SHADER(FRiveGradientPixelShader,
                        "/Plugin/Rive/Private/Rive/color_ramp.usf",
                        GLSL_colorRampFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveGradientVertexShader,
                        "/Plugin/Rive/Private/Rive/color_ramp.usf",
                        GLSL_colorRampVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveTessPixelShader,
                        "/Plugin/Rive/Private/Rive/tessellate.usf",
                        GLSL_tessellateFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveTessVertexShader,
                        "/Plugin/Rive/Private/Rive/tessellate.usf",
                        GLSL_tessellateVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRivePathPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_path.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRivePathVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_path.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(
    FRiveInteriorTrianglesPixelShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawFragmentMain,
    SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(
    FRiveInteriorTrianglesVertexShader,
    "/Plugin/Rive/Private/Rive/atomic_draw_interior_triangles.usf",
    GLSL_drawVertexMain,
    SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveImageRectPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveImageRectVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_rect.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveImageMeshPixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_mesh.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveImageMeshVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_draw_image_mesh.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveAtomicResolvePixelShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawFragmentMain,
                        SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRiveAtomicResolveVertexShader,
                        "/Plugin/Rive/Private/Rive/atomic_resolve_pls.usf",
                        GLSL_drawVertexMain,
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FRiveBltU32AsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_u32_as_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveBltU324AsF4PixelShader,
                        "/Plugin/Rive/Private/Rive/blt_u324_to_f4.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FRiveVisualizeBufferPixelShader,
                        "/Plugin/Rive/Private/Rive/visualize_buffer.usf",
                        "FragmentMain",
                        SF_Pixel);

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FFlushUniforms, "uniforms");
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FImageDrawUniforms,
                                         "imageDrawUniforms");