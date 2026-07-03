#undef PI
#include <rive/assets/file_asset.hpp>

#include "Async/Async.h"
#include "HAL/IPlatformFileModule.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Logs/RiveRendererLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "rive/renderer/ore/ore_bind_group.hpp"
#include "rive/renderer/ore/ore_buffer.hpp"
#include "rive/renderer/ore/ore_texture.hpp"
#include "rive/renderer/ore/ore_sampler.hpp"
#include "rive/renderer/ore/ore_shader_module.hpp"
#include "rive/renderer/ore/ore_pipeline.hpp"
#include "rive/renderer/ore/ore_render_pass.hpp"
#include "rive/renderer/render_canvas.hpp"
#include "rive/rive_types.hpp"
#include "rive/assets/shader_asset.hpp"
#include "Ore/RiveOrderShaderHandler.h"
#include "rive/renderer/ore/hlsl_struct_layout.hpp"
#include <rive/renderer/ore/ore_rstb_entry_container.hpp>

// Shader compilation is editor/cook-only. The shader compiler, IShaderFormat
// and TargetPlatform modules don't exist in a packaged game. At runtime in a
// cooked build the Ore shaders are loaded as precompiled bytecode (see the
// cook path), so none of this is referenced there.
#if WITH_EDITOR
#include "GlobalShader.h"
#include "ShaderCore.h" // InvalidateShaderFileCacheEntry
#include "ShaderCompiler.h"
#include "ShaderCompilerCore.h"
#include "ShaderCompilerJobTypes.h"
#include "ShaderParameterMetadataBuilder.h"
#include "Interfaces/IShaderFormat.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "RHIStrings.h"
#include "DataDrivenShaderPlatformInfo.h" // IsVulkanPlatform
#include "Internationalization/Regex.h"
#include "Ore/RiveOreShader.h"
#include "Ore/RiveOreShaderType.h"
#include "rive/file.hpp"
#include "utils/no_op_factory.hpp"
#endif // WITH_EDITOR

const FString ShaderHeader = TEXT("#include \"/Engine/Public/Platform.ush\"\n");

rive::rcp<FRiveOreShaderHandler> GRiveOreShaderHandler =
    rive::make_rcp<FRiveOreShaderHandler>();

FString FRiveOreShaderHandler::ShaderPath = TEXT("/Private/Ore");

const TCHAR* FRiveOreShaderHandler::GetLongLivedName(const char* name)
{
    auto Converter = StringCast<TCHAR>(name);
    const auto Length = Converter.Length();
    FString NewString = Converter.Get();
    const auto ID = LongLivedStrings.Add(MoveTemp(NewString));
    return *LongLivedStrings[ID];
}

const FString VirtualPath = TEXT("/Plugin/Rive/Private/Ore");

#if WITH_EDITOR
bool SaveStringToFile(const FString& Data, const FString& FileName)
{
    FString PluginShaderDir = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("Rive"))->GetBaseDir(),
        TEXT("Shaders"));

    FString SaveDirectory = PluginShaderDir / FRiveOreShaderHandler::ShaderPath;
    FString FullPath = SaveDirectory / FileName;

    // Ensure directory exists
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SaveDirectory))
    {
        PlatformFile.CreateDirectoryTree(*SaveDirectory);
    }

    return FFileHelper::SaveStringToFile(
        Data,
        *FullPath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
#endif // WITH_EDITOR

FRiveOreShaderHandler::FRiveOreShaderHandler() {}

static FArchive& SerializeSlotMap(FArchive& Ar, TMap<uint16, uint16>& Map)
{
    int32 Num = Map.Num();
    Ar << Num;
    if (Ar.IsLoading())
    {
        Map.Empty(Num);
        for (int32 i = 0; i < Num; ++i)
        {
            uint16 Key = 0, Value = 0;
            Ar << Key;
            Ar << Value;
            Map.Add(Key, Value);
        }
    }
    else
    {
        for (auto& Pair : Map)
        {
            uint16 Key = Pair.Key;
            uint16 Value = Pair.Value;
            Ar << Key;
            Ar << Value;
        }
    }
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FOreVulkanSlotRemap& Remap)
{
    SerializeSlotMap(Ar, Remap.Texture);
    SerializeSlotMap(Ar, Remap.Sampler);
    SerializeSlotMap(Ar, Remap.Uniform);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRiveOreShaderBlob& Blob)
{
    Ar << Blob.Code;
    SerializeOreShaderHash(Ar, Blob.Hash);
    Ar << Blob.SlotRemap;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRiveOreShaderModuleData& Data)
{
    // Serialize the (entry, frequency) -> blob map explicitly: the key is a
    // TPair<FString, EShaderFrequency> and the enum isn't directly archivable.
    int32 Num = Data.Shaders.Num();
    Ar << Num;
    if (Ar.IsLoading())
    {
        Data.Shaders.Empty(Num);
        for (int32 i = 0; i < Num; ++i)
        {
            FString Entry;
            uint8 Frequency = 0;
            FRiveOreShaderBlob Blob;
            Ar << Entry;
            Ar << Frequency;
            Ar << Blob;
            Data.Shaders.Add(TPair<FString, EShaderFrequency>(
                                 MoveTemp(Entry),
                                 static_cast<EShaderFrequency>(Frequency)),
                             MoveTemp(Blob));
        }
    }
    else
    {
        for (auto& Pair : Data.Shaders)
        {
            FString Entry = Pair.Key.Key;
            uint8 Frequency = static_cast<uint8>(Pair.Key.Value);
            Ar << Entry;
            Ar << Frequency;
            Ar << Pair.Value;
        }
    }
    return Ar;
}

void FRiveOreShaderHandler::registerCookedShaders(uint32_t Id,
                                                  FRiveOreShaderModuleData Data)
{
    // Mirror the editor compile path's ShaderMap write: defer to the render
    // thread so it's ordered with makeShaderModule's reads.
    ENQUEUE_RENDER_COMMAND(FRiveRegisterCookedOreShaders)
    ([this, Id, Data = MoveTemp(Data)](FRHICommandList&) {
        ShaderMap.Add(Id, Data);
    });
}

#if WITH_EDITOR
// SPIRV-Cross names resources and temporaries like `_16`. UE's shader-parameter
// parser only begins a parameter name on an alpha character (its bIsLetter
// excludes '_'), so a leading-underscore declaration is never recognized and so
// never converted to bindless which then trips Metal MSC's "all resources
// must go through the descriptor heap" assert (NumSRVs/UAVs/Samplers == 0).
// Prefix every identifier that starts with `_<digit>` with a letter so the
// parser sees it. This is a deterministic, source-wide token substitution
// (each original token maps to exactly one renamed token), so declarations,
// uses, the register() decorations and the reflected names all stay consistent.
// It is harmless on D3D, which only ever read these names back.
static FString PrefixSpirvCrossIdentifiers(const FString& Hlsl)
{
    const FRegexPattern Pattern(TEXT("([^A-Za-z0-9_]|^)_([0-9])"));
    FRegexMatcher Matcher(Pattern, Hlsl);
    FString Result;
    int32 Cursor = 0;
    while (Matcher.FindNext())
    {
        const int32 Begin = Matcher.GetMatchBeginning();
        const int32 End = Matcher.GetMatchEnding();
        Result.Append(Hlsl.Mid(Cursor, Begin - Cursor));
        Result.Append(Matcher.GetCaptureGroup(1)); // boundary char (or empty)
        Result.Append(TEXT("rive_"));
        Result.Append(Matcher.GetCaptureGroup(2)); // the leading digit
        Cursor = End;
    }
    Result.Append(Hlsl.Mid(Cursor));
    return Result;
}

FRiveOreShaderType* MakeShaderTypeFromView(const rive::ore::RstbEntryView& view,
                                           const FString& FileName,
                                           EShaderFrequency Frequency,
                                           FShaderParametersMetadata* Metadata,
                                           FString* OutSource = nullptr)
{
    if (view.source == nullptr || view.physical.empty())
    {
        UE_LOG(
            LogRiveRenderer,
            Error,
            TEXT(
                "Rive Shader Asset has a shader entry with no source or entry point!"));
        return nullptr;
    }
    auto SourceConverter =
        StringCast<TCHAR>(reinterpret_cast<const char*>(view.source),
                          view.sourceSize);
    FString Source;
    Source.Append(ShaderHeader);
    Source.Append(SourceConverter.Get(), SourceConverter.Length());

    // SPIRV-Cross emits vertex inputs (and inter-stage varyings) with
    // "TEXCOORD" semantics, but UE's D3D RHI always builds vertex input layouts
    // with the "ATTRIBUTE" semantic (see D3D12VertexDeclaration.cpp and
    // makePipeline, which sets FVertexElement::AttributeIndex = attr.shaderSlot
    // -> ATTRIBUTE<slot>). Without this rename, CreateInputLayout fails: the VS
    // input signature asks for TEXCOORD0 while the declaration provides
    // ATTRIBUTE0. The SemanticIndex is preserved, so TEXCOORD<slot> ->
    // ATTRIBUTE<slot> matches the layout. Applied to both stages so the
    // VS-output/PS-input varyings stay consistent.
    Source.ReplaceInline(TEXT("TEXCOORD"), TEXT("ATTRIBUTE"));

    // Give SPIRV-Cross's `_<digit>` identifiers a letter prefix so UE's shader-
    // parameter parser recognizes the resource declarations (and can convert
    // them to bindless on Metal MSC). See PrefixSpirvCrossIdentifiers.
    Source = PrefixSpirvCrossIdentifiers(Source);

    if (!SaveStringToFile(Source, FileName))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to save vertex shader"));
        return nullptr;
    }

    if (OutSource != nullptr)
    {
        *OutSource = Source;
    }

    auto EntryConverter = StringCast<TCHAR>(view.physical.data());

    auto TypeLayout = FRiveOreShader::StaticGetTypeLayout();

    return new FRiveOreShaderType(FileName,
                                  VirtualPath / FileName,
                                  EntryConverter.Get(),
                                  Frequency,
                                  TypeLayout,
                                  Metadata);
}

// One vertex/fragment shader parsed out of a Rive ShaderAsset, ready to feed
// to FGlobalShaderTypeCompiler::BeginCompileShader.
struct FParsedOreShader
{
    FString Entry;
    EShaderFrequency Frequency;
    FRiveOreShaderType* Type;
    // The HLSL fed to the compiler, kept so the Vulkan slot remap can be built
    // from its register() decorations (joined to the reflected ParameterMap).
    FString HlslSource;
};

// Decodes a ShaderAsset's sidecar blobs into ready to compile
// FRiveOreShaderType objects one per vertex/fragment view writing each view's
// `.usf` to the plugin shader dir as a side effect (via
// MakeShaderTypeFromView). Platform-agnostic: Returns false on any parse
// failure (already logged); OutShaders is only populated on success.
static bool ParseShaderAsset(rive::ShaderAsset* shaderAsset,
                             const FString& AssetName,
                             TArray<FParsedOreShader>& OutShaders)
{
    // 3 is the hlsl index
    auto hlslBlob = shaderAsset->findShader(3);
    if (hlslBlob.empty())
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to find hlsl blob"));
        return false;
    }

    // Binding map is at 12. Parsed here only to validate the sidecar is
    // well-formed; the per-stage native slots are consumed later by the
    // backend's makeBindGroup, not during type construction.
    auto bindingMapBlob = shaderAsset->findShader(12);
    if (bindingMapBlob.empty())
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to find binding map"));
        return false;
    }

    rive::ore::BindingMap bindingMap;
    const bool ok = rive::ore::BindingMap::fromBlob(bindingMapBlob.data(),
                                                    bindingMapBlob.size(),
                                                    &bindingMap);
    checkf(ok, TEXT("binding-map sidecar failed to parse"));

    auto hlslStructBlob = shaderAsset->findShader(17);
    if (hlslStructBlob.empty())
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to find hlsl struct blob"));
        return false;
    }

    rive::ore::HLSLStructLayout hlslStruct;
    if (!rive::ore::HLSLStructLayout::fromBlob(hlslStructBlob.data(),
                                               hlslStructBlob.size(),
                                               &hlslStruct))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to parse hlsl struct layout"));
        return false;
    }

    std::vector<rive::ore::RstbEntryView> views;
    if (!parsePerEntryContainer(hlslBlob.data(), hlslBlob.size(), views))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Shader Asset failed to parse rstb container"));
        return false;
    }

    for (const auto& v : views)
    {
        EShaderFrequency Stage;
        FString FileName;
        auto EntryConverter = StringCast<TCHAR>(v.physical.c_str());
        const FString Entry = EntryConverter.Get();
        switch (v.stage)
        {
            case 0:
                Stage = SF_Vertex;
                FileName = AssetName + TEXT("_") + Entry +
                           FString(TEXT("_vertex.usf"));
                break;
            case 1:
                Stage = SF_Pixel;
                FileName = AssetName + TEXT("_") + Entry +
                           FString(TEXT("_fragment.usf"));
                break;
            default:
                // only support vertex and fragment shaders for now, if we
                // see anything else just skip it.
                continue;
        }

        // No FShaderParametersMetadata is attached (nullptr). We compile these
        // synchronously via the IShaderFormat (see CompileOreShaderToBlob) and
        // never insert them into the global shader map, so the DDC freeze that
        // previously required the metadata is gone. A null
        // RootParametersStructure also means the compiler does no
        // reflection/parameter-map binding, so the both-stages "no mapping"
        // mismatch can't occur. The runtime path binds every resource by
        // native register slot via the BindingMap.
        FString HlslSource;
        auto ShaderType =
            MakeShaderTypeFromView(v, FileName, Stage, nullptr, &HlslSource);
        OutShaders.Add({Entry, Stage, ShaderType, MoveTemp(HlslSource)});
    }

    return true;
}

// Compiles a single parsed Ore shader to its UE-framed bytecode blob, WITHOUT
// going through the global shader map. This mirrors the engine's
// PrepareGlobalShaderCompileJob (SetupCompileEnvironment +
// GlobalBeginCompileShader) to build a correct FShaderCompilerInput, then runs
// the exported PreprocessShader + CompileShader free functions synchronously on
// the calling thread Editor/dev-only (no shader compiler exists in cooked
// builds). Returns false on any failure (already logged); OutBlob is only valid
// on success.
static void BuildReflectionSlotRemap(const FString& Hlsl,
                                     const FShaderParameterMap& ParamMap,
                                     FOreVulkanSlotRemap& Out)
{
    // Parse `<name> : register(<letter><number> ...)` -> name -> register
    // number.
    TMap<FString, uint16> NameToRegister;
    const FRegexPattern Pattern(
        TEXT("(\\w+)\\s*:\\s*register\\s*\\(\\s*[tsbu](\\d+)"));
    FRegexMatcher Matcher(Pattern, Hlsl);
    while (Matcher.FindNext())
    {
        const FString Name = Matcher.GetCaptureGroup(1);
        const int32 Reg = FCString::Atoi(*Matcher.GetCaptureGroup(2));
        NameToRegister.Add(Name, static_cast<uint16>(Reg));
    }

    auto Join = [&](EShaderParameterType Type,
                    bool bUseBufferIndex,
                    const TCHAR* KindName,
                    TMap<uint16, uint16>& OutMap) {
        for (const FStringView NameView :
             ParamMap.GetAllParameterNamesOfType(Type))
        {
            const FString Name(NameView);
            uint16 BufferIndex = 0, BaseIndex = 0, Size = 0;
            if (!ParamMap.FindParameterAllocation(Name,
                                                  BufferIndex,
                                                  BaseIndex,
                                                  Size))
            {
                continue;
            }
            const uint16 BindIndex = bUseBufferIndex ? BufferIndex : BaseIndex;
            const uint16* Reg = NameToRegister.Find(Name);
            if (Reg == nullptr)
            {
                UE_LOG(LogRiveRenderer,
                       Warning,
                       TEXT("Ore Vulkan remap: %s '%s' (bind=%u) has no "
                            "matching register() in HLSL — binding will fall "
                            "back to pass-through and may crash."),
                       KindName,
                       *Name,
                       BindIndex);
                continue;
            }
            OutMap.Add(*Reg, BindIndex);
        }
    };

    Join(EShaderParameterType::SRV, false, TEXT("SRV"), Out.Texture);
    Join(EShaderParameterType::Sampler, false, TEXT("Sampler"), Out.Sampler);
    Join(EShaderParameterType::UniformBuffer, true, TEXT("UB"), Out.Uniform);

    // Bindless platforms reflect the (register-stripped) textures and samplers
    // under their Bindless* types. BaseIndex is the resource's bindless
    // parameter index in the reflected table. This mirrors the SRV/Sampler join
    // above. The exact runtime binding semantics for bindless are finalized
    // when the Metal RHI opts into the remap (GRHIOreNeedsReflectionSlotRemap).
    Join(EShaderParameterType::BindlessSRV,
         false,
         TEXT("BindlessSRV"),
         Out.Texture);
    Join(EShaderParameterType::BindlessSampler,
         false,
         TEXT("BindlessSampler"),
         Out.Sampler);
}

static bool CompileOreShaderToBlob(FRiveOreShaderType* ShaderType,
                                   EShaderPlatform Platform,
                                   const FString& HlslSource,
                                   FRiveOreShaderBlob& OutBlob)
{
    FShaderCompileJob Job;
#if RIVE_ORE_NEW_SHADER_COMPILE_API
    Job.Key = FShaderCompileJobKey(ShaderType, Platform, nullptr, 0);
#else
    Job.Key = FShaderCompileJobKey(ShaderType, nullptr, 0);
#endif
    Job.bIsGlobalShader = true;
    Job.bErrorsAreLikelyToBeCode = true;

    // Let the shader type populate permutation/platform defines, then fill the
    // rest of the input exactly as the global-shader path does.
    ShaderType->SetupCompileEnvironment(Platform,
                                        0,
                                        EShaderPermutationFlags::None,
                                        Job.Input.Environment);

    GlobalBeginCompileShader(
        TEXT("Global"),
        nullptr, // VFType
        ShaderType,
        nullptr, // ShaderPipelineType
        0,       // PermutationId
        ShaderType->GetShaderFilename(),
        ShaderType->GetFunctionName(),
        FShaderTarget(ShaderType->GetFrequency(), Platform),
        Job.Input);

    // GlobalBeginCompileShader resolves whether this platform compiles with
    // bindless (UE::ShaderCompiler::ShouldCompileWithBindlessEnabled). On
    // bindless platforms (e.g. Metal) the compiler's reflection requires
    // every SRV/UAV/sampler to go through the descriptor heap and asserts if
    // any resource keeps a real register. UE skips its bindless rewrite
    // for declarations that already carry register(). Feed the compiler a
    // register-stripped copy so the rewrite applies; D3D keeps the
    // original register() bindings.
    //
    // The root source MUST be changed on disk: the preprocessor loads it
    // through the on-disk shader-file cache (GetShaderPreprocessDependencies ->
    // GShaderFileCache), which bypasses
    // Environment.IncludeVirtualPathToContentsMap (that map only overrides
    // *include* files). So rewrite the .usf with the platform-correct source
    // and drop the stale cache entry before preprocessing. We always rewrite
    // (not just on bindless) so a multi-format cook that already wrote the
    // stripped variant for one format restores the original for the next; the
    // file/cache pair is left in a state matching this compile.
    FString DiskSource = HlslSource;
    if (Job.Input.bBindlessEnabled)
    {
        const auto Utf8 = StringCast<ANSICHAR>(*HlslSource);
        const std::string Stripped = rive::ore::stripHLSLRegisterAnnotations(
            std::string(Utf8.Get(), Utf8.Length()));
        DiskSource = StringCast<TCHAR>(Stripped.c_str()).Get();
    }
    SaveStringToFile(DiskSource,
                     FPaths::GetCleanFilename(ShaderType->GetShaderFilename()));
    InvalidateShaderFileCacheEntry(*Job.Input.VirtualSourceFilePath, Platform);

    UE_LOG(
        LogRiveRenderer,
        Verbose,
        TEXT(
            "Rive Ore shader '%s' (%s): bindless=%d (register decorations %s)"),
        ShaderType->GetName(),
        *Job.Input.ShaderFormat.ToString(),
        Job.Input.bBindlessEnabled ? 1 : 0,
        Job.Input.bBindlessEnabled ? TEXT("stripped") : TEXT("kept"));

    // GlobalBeginCompileShader sets a compression format (Oodle in the editor),
    // which makes CompileShader compress Output.ShaderCode. The engine's normal
    // path decompresses inside the shader map before RHICreate*Shader; we feed
    // the blob straight to the RHI, so it must stay uncompressed or D3D12's
    // FShaderCodeReader parses the compressed bytes as framing and asserts on a
    // garbage optional-data size. Force no compression for this in-process
    // path.
    Job.Input.CompressionFormat = NAME_None;

    const IShaderFormat* Format =
        GetTargetPlatformManagerRef().FindShaderFormat(Job.Input.ShaderFormat);
    if (!ensureMsgf(Format != nullptr,
                    TEXT("No IShaderFormat for %s"),
                    *Job.Input.ShaderFormat.ToString()))
    {
        return false;
    }

    if (!PreprocessShader(&Job))
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Ore shader '%s' failed preprocessing"),
               ShaderType->GetName());
        return false;
    }

    TArray<const IShaderFormat*> Formats;
    Formats.Add(Format);
#if RIVE_ORE_NEW_SHADER_COMPILE_API
    CompileShader(Formats, Job);
#else
    // 5.7 and earlier take an explicit working directory; mirror the engine's
    // own direct-compile path
    // (FShaderCompileUtilities::ExecuteShaderCompileJob).
    CompileShader(Formats, Job, FString(FPlatformProcess::ShaderDir()));
#endif

    if (!Job.Output.bSucceeded)
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Rive Ore shader '%s' failed to compile:"),
               ShaderType->GetName());
        for (const FShaderCompilerError& Error : Job.Output.Errors)
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("  %s"),
                   *Error.GetErrorStringWithLineMarker());
        }
        return false;
    }

    const TConstArrayView<uint8> CodeView = Job.Output.ShaderCode.GetReadView();
    OutBlob.Code.Append(CodeView.GetData(), CodeView.Num());
    OutBlob.Hash = Job.Output.OutputHash;

    // Some RHIs bind resources by a flat per-stage descriptor index (the
    // resource's position in the reflected resource table) rather than by HLSL
    // register, so build the register->bind-index remap from this stage's
    // reflection. The remap is built for every platform but only consumed by
    // those RHIs (see GRHIOreNeedsReflectionSlotRemap / setBindGroup). On
    // register-binding RHIs (D3D) it is inert. Building it here, rather
    // than gating on the target platform, keeps the cook free of any
    // platform-specific knowledge.
    BuildReflectionSlotRemap(HlslSource,
                             Job.Output.ParameterMap,
                             OutBlob.SlotRemap);

    return true;
}

namespace
{
// A headless rive Factory so the cook path can import a .riv with no RHI.
// Mirrors rive::NoOpFactory (whose .cpp isn't compiled into the UE
// RiveLibrary): the render objects are no-ops, which is all File::import needs
// to reach the in-band shader assets.
class FOreNoOpRenderImage : public rive::RenderImage
{};
class FOreNoOpRenderShader : public rive::RenderShader
{};
class FOreNoOpRenderPaint : public rive::RenderPaint
{
public:
    void color(unsigned int) override {}
    void style(rive::RenderPaintStyle) override {}
    void thickness(float) override {}
    void join(rive::StrokeJoin) override {}
    void cap(rive::StrokeCap) override {}
    void blendMode(rive::BlendMode) override {}
    void shader(rive::rcp<rive::RenderShader>) override {}
    void invalidateStroke() override {}
    void feather(float) override {}
};
class FOreNoOpRenderPath : public rive::RenderPath
{
public:
    void rewind() override {}
    void fillRule(rive::FillRule) override {}
    void addPath(rive::CommandPath*, const rive::Mat2D&) override {}
    void addRenderPath(const rive::RenderPath*, const rive::Mat2D&) override {}
    void moveTo(float, float) override {}
    void lineTo(float, float) override {}
    void cubicTo(float, float, float, float, float, float) override {}
    void close() override {}
    void addRawPath(const rive::RawPath&) override {}
};
class FOreHeadlessFactory : public rive::Factory
{
public:
    rive::rcp<rive::RenderBuffer> makeRenderBuffer(rive::RenderBufferType,
                                                   rive::RenderBufferFlags,
                                                   size_t) override
    {
        return nullptr;
    }
    rive::rcp<rive::RenderShader> makeLinearGradient(float,
                                                     float,
                                                     float,
                                                     float,
                                                     const rive::ColorInt[],
                                                     const float[],
                                                     size_t) override
    {
        return rive::make_rcp<FOreNoOpRenderShader>();
    }
    rive::rcp<rive::RenderShader> makeRadialGradient(float,
                                                     float,
                                                     float,
                                                     const rive::ColorInt[],
                                                     const float[],
                                                     size_t) override
    {
        return rive::make_rcp<FOreNoOpRenderShader>();
    }
    rive::rcp<rive::RenderPath> makeRenderPath(rive::RawPath&,
                                               rive::FillRule) override
    {
        return rive::make_rcp<FOreNoOpRenderPath>();
    }
    rive::rcp<rive::RenderPath> makeEmptyRenderPath() override
    {
        return rive::make_rcp<FOreNoOpRenderPath>();
    }
    rive::rcp<rive::RenderPaint> makeRenderPaint() override
    {
        return rive::make_rcp<FOreNoOpRenderPaint>();
    }
    rive::rcp<rive::RenderImage> decodeImage(rive::Span<const uint8_t>) override
    {
        return nullptr;
    }
};

// Headless asset loader for the cook path: decodes shader assets during a
// File::import and keeps them (with their asset ids) so they can be compiled
// synchronously afterwards. All other asset types are ignored.
class FOreCookShaderCapture : public rive::FileAssetLoader
{
public:
    struct Captured
    {
        uint32_t AssetId;
        rive::rcp<rive::ShaderAsset> Asset;
    };
    std::vector<Captured> Shaders;

    bool loadContents(rive::FileAsset& asset,
                      rive::Span<const uint8_t> inBandBytes,
                      rive::Factory* factory) override
    {
        if (asset.is<rive::ShaderAsset>())
        {
            auto* ShaderAsset = asset.as<rive::ShaderAsset>();
            if (ShaderAsset != nullptr &&
                ShaderAsset->decode(inBandBytes, factory))
            {
                Shaders.push_back(
                    {asset.assetId(), rive::ref_rcp(ShaderAsset)});
            }
        }
        // Never replace the asset.
        return false;
    }
};
} // namespace

bool FRiveOreShaderHandler::compileFileShadersForCook(
    const uint8* RivData,
    int32 RivSize,
    const ITargetPlatform* TargetPlatform,
    TMap<FName, TMap<uint32_t, FRiveOreShaderModuleData>>& OutModulesByFormat)
{
    if (RivData == nullptr || RivSize <= 0 || TargetPlatform == nullptr)
    {
        return false;
    }

    // Cook for every shader format the target platform ships (e.g. a desktop
    // target may carry both SM5 and SM6; the runtime then picks whichever
    // matches the RHI it ends up running on). The runtime ShaderMap stays keyed
    // by asset id only — PostLoad selects the one format up front.
    TArray<FName> Formats;
    TargetPlatform->GetAllTargetedShaderFormats(Formats);
    if (Formats.Num() == 0)
    {
        return false;
    }

    // Import the .riv headlessly (no RHI), decoding its shader assets. The
    // parse is format-independent, so do it once and recompile per format.
    FOreHeadlessFactory Factory;
    auto Capture = rive::make_rcp<FOreCookShaderCapture>();
    rive::ImportResult Result = rive::ImportResult::malformed;
    rive::rcp<rive::File> File = rive::File::import(
        rive::Span<const uint8_t>(RivData, static_cast<size_t>(RivSize)),
        &Factory,
        &Result,
        Capture.get());

    if (Result != rive::ImportResult::success)
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Cook: failed to import .riv for Ore shaders (result %d)"),
               static_cast<int32>(Result));
        return false;
    }

    // Parse each asset once (format-independent). Recompile its shaders for
    // every targeted format.
    struct FParsedAsset
    {
        uint32_t AssetId;
        TArray<FParsedOreShader> Shaders;
    };
    TArray<FParsedAsset> ParsedAssets;
    for (const FOreCookShaderCapture::Captured& Captured : Capture->Shaders)
    {
        auto NameConverter = StringCast<TCHAR>(Captured.Asset->name().c_str());
        const FString Name = NameConverter.Get();

        TArray<FParsedOreShader> ShaderTypes;
        if (!ParseShaderAsset(Captured.Asset.get(), Name, ShaderTypes))
        {
            continue;
        }
        ParsedAssets.Add({Captured.AssetId, MoveTemp(ShaderTypes)});
    }

    for (const FName Format : Formats)
    {
        const EShaderPlatform Platform =
            ShaderFormatToLegacyShaderPlatform(Format);

        TMap<uint32_t, FRiveOreShaderModuleData> Modules;
        for (const FParsedAsset& Asset : ParsedAssets)
        {
            FRiveOreShaderModuleData Data;
            for (const FParsedOreShader& Parsed : Asset.Shaders)
            {
                FRiveOreShaderBlob Blob;
                if (CompileOreShaderToBlob(Parsed.Type,
                                           Platform,
                                           Parsed.HlslSource,
                                           Blob))
                {
                    Data.Shaders.Add({Parsed.Entry, Parsed.Frequency},
                                     MoveTemp(Blob));
                }
            }
            if (Data.Shaders.Num() > 0)
            {
                Modules.Add(Asset.AssetId, MoveTemp(Data));
            }
        }

        if (Modules.Num() > 0)
        {
            OutModulesByFormat.Add(Format, MoveTemp(Modules));
        }
    }
    return OutModulesByFormat.Num() > 0;
}

bool FRiveOreShaderHandler::compileFileShadersForEditor(const uint8* RivData,
                                                        int32 RivSize)
{
    check(IsInGameThread());
    if (RivData == nullptr || RivSize <= 0)
    {
        return false;
    }

    // Import the .riv headlessly (no RHI), decoding its shader assets. This is
    // the same headless path the cook uses; the parse is RHI-independent.
    FOreHeadlessFactory Factory;
    auto Capture = rive::make_rcp<FOreCookShaderCapture>();
    rive::ImportResult Result = rive::ImportResult::malformed;
    rive::rcp<rive::File> File = rive::File::import(
        rive::Span<const uint8_t>(RivData, static_cast<size_t>(RivSize)),
        &Factory,
        &Result,
        Capture.get());

    if (Result != rive::ImportResult::success)
    {
        UE_LOG(
            LogRiveRenderer,
            Error,
            TEXT("Editor: failed to import .riv for Ore shaders (result %d)"),
            static_cast<int32>(Result));
        return false;
    }

    // No Ore shader assets in this file — nothing to compile.
    if (Capture->Shaders.empty())
    {
        return false;
    }

    const EShaderPlatform Platform =
        GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

    bool bAny = false;
    for (const FOreCookShaderCapture::Captured& Captured : Capture->Shaders)
    {
        auto NameConverter = StringCast<TCHAR>(Captured.Asset->name().c_str());
        const FString Name = NameConverter.Get();

        TArray<FParsedOreShader> ShaderTypes;
        if (!ParseShaderAsset(Captured.Asset.get(), Name, ShaderTypes))
        {
            continue;
        }

        FRiveOreShaderModuleData Data;
        for (const FParsedOreShader& Parsed : ShaderTypes)
        {
            FRiveOreShaderBlob Blob;
            if (CompileOreShaderToBlob(Parsed.Type,
                                       Platform,
                                       Parsed.HlslSource,
                                       Blob))
            {
                Data.Shaders.Add({Parsed.Entry, Parsed.Frequency},
                                 MoveTemp(Blob));
            }
        }

        if (Data.Shaders.Num() > 0)
        {
            // Defer the ShaderMap write to the render thread so it is ordered
            // with makeShaderModule's reads (matches registerCookedShaders).
            // The caller enqueues this before LoadFile, so it lands first.
            const uint32_t Id = Captured.AssetId;
            ENQUEUE_RENDER_COMMAND(FRiveRegisterEditorOreShaders)
            ([this, Id, Data = MoveTemp(Data)](FRHICommandList&) {
                ShaderMap.Add(Id, Data);
            });
            bAny = true;
        }
    }

    return bAny;
}
#endif // WITH_EDITOR

bool FRiveOreShaderHandler::loadContents(rive::FileAsset& asset,
                                         rive::Span<const uint8_t> inBandBytes,
                                         rive::Factory* factory)
{
    if (asset.is<rive::ShaderAsset>())
    {
        auto shaderAsset = asset.as<rive::ShaderAsset>();
        check(shaderAsset);
        if (!shaderAsset->decode(inBandBytes, factory))
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("Rive Shader Asset failed to decode in band bytes"));
            return false;
        }

#if WITH_EDITOR
        // URiveFile::Initialize normally compiles and registers these shaders
        // synchronously before the file loads (compileFileShadersForEditor), so
        // the module is already present here. Skip the lazy async recompile in
        // that case; this path remains only as a fallback. Reading ShaderMap is
        // safe loadContents and writes both run on the render thread.
        if (ShaderModuleForId(asset.assetId()) != nullptr)
        {
            return false;
        }

        auto NameConverter = StringCast<TCHAR>(asset.name().c_str());
        const FString Name = NameConverter.Get();

        TArray<FParsedOreShader> ShaderTypes;
        if (!ParseShaderAsset(shaderAsset, Name, ShaderTypes))
        {
            return false;
        }

        AsyncTask(ENamedThreads::GameThread,
                  [Name,
                   ShaderTypes,
                   AssetId = asset.assetId(),
                   LocalMap = &ShaderMap]() {
                      const EShaderPlatform Platform =
                          GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

                      // Compile each shader straight to bytecode via the
                      // IShaderFormat, bypassing the global shader map (see
                      // CompileOreShaderToBlob).
                      TMap<TPair<FString, EShaderFrequency>, FRiveOreShaderBlob>
                          Shaders;
                      for (const FParsedOreShader& Parsed : ShaderTypes)
                      {
                          FRiveOreShaderBlob Blob;
                          if (!CompileOreShaderToBlob(Parsed.Type,
                                                      Platform,
                                                      Parsed.HlslSource,
                                                      Blob))
                          {
                              continue;
                          }
                          Shaders.Add({Parsed.Entry, Parsed.Frequency},
                                      MoveTemp(Blob));
                      }

                      ENQUEUE_RENDER_COMMAND(FShaderMapUpdateCommand)(
                          [AssetId,
                           Shaders = MoveTemp(Shaders),
                           LocalMap = LocalMap](FRHICommandList&) {
                              LocalMap->Add(AssetId, {Shaders});
                          });
                  });
#endif // WITH_EDITOR
    }

    // We never replace the asset so always return false.
    return false;
}
