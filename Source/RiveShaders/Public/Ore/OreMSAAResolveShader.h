/*
 * Copyright 2025 Rive
 */

#pragma once

#include "GlobalShader.h"
#include "ShaderParameterUtils.h"

// Fullscreen-triangle vertex shader for the manual Ore MSAA resolve. No vertex
// buffer positions come from SV_VertexID. See OreMSAAResolve.usf.
class FOreMSAAResolveVS : public FGlobalShader
{
    DECLARE_EXPORTED_GLOBAL_SHADER(FOreMSAAResolveVS, RIVESHADERS_API);

public:
    FOreMSAAResolveVS() = default;
    FOreMSAAResolveVS(
        const ShaderMetaType::CompiledShaderInitializerType& Init) :
        FGlobalShader(Init)
    {}

    static bool ShouldCompilePermutation(
        const FGlobalShaderPermutationParameters&)
    {
        return true;
    }
};

// Averages every sample of an MSAA color source into a single-sample target.
// Used on Vulkan, where UE 5.9's dynamic-rendering auto-resolve is broken.
class FOreMSAAResolvePS : public FGlobalShader
{
    DECLARE_EXPORTED_GLOBAL_SHADER(FOreMSAAResolvePS, RIVESHADERS_API);

public:
    FOreMSAAResolvePS() = default;
    FOreMSAAResolvePS(
        const ShaderMetaType::CompiledShaderInitializerType& Init) :
        FGlobalShader(Init)
    {
        SourceParameter.Bind(Init.ParameterMap, TEXT("OreResolveSource"));
    }

    static bool ShouldCompilePermutation(
        const FGlobalShaderPermutationParameters&)
    {
        return true;
    }

    LAYOUT_FIELD(FShaderResourceParameter, SourceParameter);
};
