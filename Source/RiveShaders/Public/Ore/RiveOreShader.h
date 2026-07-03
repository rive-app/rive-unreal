#pragma once
#include "GlobalShader.h"

// Editor/cook-only: this type exists solely to drive UE's shader compiler for
// Ore shaders. A packaged game has no compiler (and FGlobalShader lacks the
// compile-only members this uses), so it's excluded there — runtime uses
// precooked bytecode instead.
#if WITH_EDITOR
class FRiveOreShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveOreShader, RIVESHADERS_API);

    using FPermutationDomain = FShaderPermutationNone;
    RIVESHADERS_API static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};
#endif // WITH_EDITOR