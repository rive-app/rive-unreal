#pragma once

#include "GlobalShader.h"

// Editor/cook-only: drives UE's shader compiler for Ore shaders. Excluded from
// packaged games (no compiler, runtime uses precooked bytecode).
#if WITH_EDITOR

// The shader-compile API is different based on engine version. This is here
// for us to check which API to use.
#define RIVE_ORE_NEW_SHADER_COMPILE_API                                        \
    (ENGINE_MAJOR_VERSION > 5 ||                                               \
     (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 9))

class FRiveOreShaderType : public FGlobalShaderType
{
public:
    typedef FGlobalShaderType Super;
    FRiveOreShaderType(FString InName,
                       FString InSourceFilename,
                       FString InFunctionName,
                       uint32 InFrequency,
                       FTypeLayoutDesc& InTypeLayout,
                       FShaderParametersMetadata* InShaderParameterMetaData);

    const FShaderBindingLayout* GetShaderBindingLayout(
        const FShaderPermutationParameters&) const
    {
        return nullptr;
    }

private:
    TUniquePtr<FShaderParametersMetadata> ShaderParametersMetadata;
    const FString Name;
    const FString SourceFilename;
    const FString FunctionName;
};
#endif // WITH_EDITOR