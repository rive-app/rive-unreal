#include "Ore/RiveOreShaderType.h"
#include "Ore/RiveOreShader.h"

#if WITH_EDITOR
// These types exist only to drive the manual CompileOreShaderToBlob path;
// they are registered in the global shader type list as a side effect of
// construction. Returning false here keeps them out of FGlobalShaderMapId and
// CompileGlobalShaderMap, so the cooker neither hashes them into the global
// shader map key nor compiles them into the target's global shader map (whose
// runtime would reject shader types that don't exist in a packaged game).
static bool NeverCompilePermutation(const FShaderPermutationParameters&)
{
    return false;
}

FShader* ConstructCompiledInstance(
    const typename FShader::CompiledShaderInitializerType& Initializer)
{
    return new FRiveOreShader(
        static_cast<
            const typename FRiveOreShaderType::CompiledShaderInitializerType&>(
            Initializer));
}

FRiveOreShaderType::FRiveOreShaderType(
    FString InName,
    FString InSourceFilename,
    FString InFunctionName,
    uint32 InFrequency,
    FTypeLayoutDesc& InTypeLayout,
    FShaderParametersMetadata* InShaderParameterMetaData) :
    FGlobalShaderType(InTypeLayout,
                      *InName,
                      *InSourceFilename,
                      *InFunctionName,
                      InFrequency,
                      FGlobalShader::FPermutationDomain::PermutationCount,
#if RIVE_ORE_NEW_SHADER_COMPILE_API
                      FRiveOreShader::FPermutationDomain::SpecializationCount,
#endif
                      FRiveOreShader::ConstructSerializedInstance,
                      ConstructCompiledInstance,
                      NeverCompilePermutation,
                      FRiveOreShader::ShouldPrecachePermutationImpl,
#if RIVE_ORE_NEW_SHADER_COMPILE_API
                      FRiveOreShader::GetUnspecializedIdImpl,
                      FRiveOreShader::GetSpecializationValuesImpl,
#endif
                      FRiveOreShader::GetRayTracingPayloadType,
                      FRiveOreShader::GetShaderBindingLayout,
                      FRiveOreShader::ModifyCompilationEnvironmentImpl,
                      FRiveOreShader::ValidateCompiledResult,
                      FRiveOreShader::GetOverrideJobPriority,
                      sizeof(FRiveOreShader),
                      InShaderParameterMetaData,
                      FRiveOreShader::GetPermutationIdStringImpl),
    ShaderParametersMetadata(InShaderParameterMetaData),
    Name(MoveTemp(InName)),
    SourceFilename(MoveTemp(InSourceFilename)),
    FunctionName(MoveTemp(InFunctionName))
{}
#endif // WITH_EDITOR