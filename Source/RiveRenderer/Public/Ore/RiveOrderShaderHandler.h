#pragma once
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"
#include "CoreMinimal.h"
#include "Shader.h"
#include "Ore/OreShaderHash.h"
#include "Ore/OreSlotRemap.h"

class URiveFile;
THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/refcnt.hpp"
#include <rive/file_asset_loader.hpp>
THIRD_PARTY_INCLUDES_END

class FRiveOreShaderType;
class ITargetPlatform;

// One compiled shader, captured straight out of the UE compile job. `Code` is
// the UE-framed shader microcode (FShaderCode layout: bytecode + packed
// resource counts in the optional-data trailer) exactly what RHICreate*Shader
// consumes. No FShader / global-shader-map object is kept. The runtime path
// rebuilds the RHI shader from this blob and binds purely by slot.
struct FRiveOreShaderBlob
{
    TArray<uint8> Code;
    // Matches FShaderCompilerOutput::OutputHash and RHICreate*Shader's hash
    // parameter. The concrete type is engine-version dependent.
    FOreShaderHash Hash;
    // register->bind-index translation (empty on D3D). Built
    // at cook time from this stage's reflected ParameterMap.
    FOreVulkanSlotRemap SlotRemap;
};

struct FRiveOreShaderModuleData
{
    // Map of (entry point, frequency) to compiled bytecode blob.
    TMap<TPair<FString, EShaderFrequency>, FRiveOreShaderBlob> Shaders;
};

// Serialization for cooking precompiled Ore shaders into a packaged build (no
// shader compiler exists at runtime there). Exported so URiveFile (Rive module)
// can read/write them in its own Serialize().
RIVERENDERER_API FArchive& operator<<(FArchive& Ar, FRiveOreShaderBlob& Blob);
RIVERENDERER_API FArchive& operator<<(FArchive& Ar,
                                      FRiveOreShaderModuleData& Data);

// Class used to intercept shaders from rive scripts and write them to disk
// while maintaining a map to them.
class FRiveOreShaderHandler : public rive::FileAssetLoader
{
public:
    FRiveOreShaderHandler();

    virtual bool loadContents(rive::FileAsset& asset,
                              rive::Span<const uint8_t> inBandBytes,
                              rive::Factory* factory) override;

    const FRiveOreShaderModuleData* ShaderModuleForId(uint32_t Id) const
    {
        return ShaderMap.Find(Id);
    }

    // Registers precompiled (cooked) shaders for an asset id so the runtime can
    // build RHI shaders without a compiler. Called by URiveFile in a packaged
    // build before its .riv is processed. Thread-safe. Defers the ShaderMap
    // write to the render thread, matching the editor compile path.
    RIVERENDERER_API void registerCookedShaders(uint32_t Id,
                                                FRiveOreShaderModuleData Data);

#if WITH_EDITOR
    // Cook-time: imports the given .riv bytes headlessly and compiles each Ore
    // shader asset for *every* shader format the target platform targets (e.g.
    // SM5 + SM6, or multiple mobile formats), keyed by shader-format name then
    // asset id. At runtime PostLoad selects the format matching the running
    // RHI. Synchronous; safe to call from a cook commandlet (no RHI required).
    // Editor/cook-only.
    RIVERENDERER_API bool compileFileShadersForCook(
        const uint8* RivData,
        int32 RivSize,
        const ITargetPlatform* TargetPlatform,
        TMap<FName, TMap<uint32_t, FRiveOreShaderModuleData>>&
            OutModulesByFormat);

    // Editor live path: imports the given .riv bytes headlessly, compiles each
    // Ore shader asset for the running RHI's shader platform, and registers the
    // results in ShaderMap (deferred to the render thread, like
    // registerCookedShaders). Synchronous on the calling game thread.
    //
    // Call immediately before the file's native LoadFile so the registration
    // render command is enqueued ahead of the artboard commands that call
    // makeShaderModule. The lazy compile in loadContents (render -> game ->
    // render) otherwise loses that race when the file is drawn the instant it
    // loads (e.g. the UMG widget preview). Editor/dev-only.
    RIVERENDERER_API bool compileFileShadersForEditor(const uint8* RivData,
                                                      int32 RivSize);
#endif

    static FString ShaderPath;

    const TCHAR* GetLongLivedName(const std::string& name)
    {
        return GetLongLivedName(name.data());
    }

    const TCHAR* GetLongLivedName(const char* name);

private:
    // Unreal normally uses static const strings for shader names / types. This
    // is an issue for us because we are loading the string names at runtime. To
    // solve this issue, we hold onto all strings needed for shader params etc..
    // here. Ideally, this would be owned by the file that loaded this shader
    // but that causes a recursive dependency.
    TSet<FString> LongLivedStrings;

    TMap<uint32_t, FRiveOreShaderModuleData> ShaderMap;
};

extern RIVERENDERER_API rive::rcp<FRiveOreShaderHandler> GRiveOreShaderHandler;