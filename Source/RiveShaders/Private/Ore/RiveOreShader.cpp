#include "Ore/RiveOreShader.h"

#include <CoreMinimal.h>
#include "GlobalShader.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"
#include "Interfaces/IPluginManager.h"

#if WITH_EDITOR
void FRiveOreShader::ModifyCompilationEnvironment(
    const FShaderPermutationParameters&,
    FShaderCompilerEnvironment& Environment)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4 && !PLATFORM_APPLE
    // We are not bindless so this flag must be added for vulkan to work
    Environment.CompilerFlags.Add(CFLAG_ForceBindful);
#endif
}

IMPLEMENT_GLOBAL_SHADER(FRiveOreShader,
                        "/Plugin/Rive/Private/Ore/dummyShader.usf",
                        "main",
                        SF_Vertex);
#endif // WITH_EDITOR
