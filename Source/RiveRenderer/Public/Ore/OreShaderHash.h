#pragma once

#include "CoreMinimal.h"

// Portable shader-hash type for the Ore backend across UE 5.6 - 5.9.
//
// UE 5.9 introduced an xxHash64-based FShaderHash
// (Core/Public/Hash/ShaderHash.h) and routes it through
// FShaderCompilerOutput::OutputHash and the RHICreate*Shader(Code, Hash)
// overloads. Earlier engines (5.6 - 5.8) use the SHA-1 FSHAHash for that same
// role. We alias FOreShaderHash to whichever the running engine provides, so
// the call sites that touch it bind without any per-version branching:
//   * OutBlob.Hash = Job.Output.OutputHash;            (cook time)
//   * RHICreate{Vertex,Pixel}Shader(Blob->Code, Blob->Hash);  (runtime)
//
// The header's presence is detected directly (rather than gating on an exact
// engine minor version) because the API moved during the 5.8/5.9 window.
#if __has_include("Hash/ShaderHash.h")
#include "Hash/ShaderHash.h"
using FOreShaderHash = FShaderHash;
#define RIVE_ORE_SHADER_HASH_IS_XXHASH64 1
#else
#include "Misc/SecureHash.h" // FSHAHash
using FOreShaderHash = FSHAHash;
#define RIVE_ORE_SHADER_HASH_IS_XXHASH64 0
#endif

// Portable (de)serialization of an Ore shader-blob hash. The xxHash64
// FShaderHash and the SHA-1 FSHAHash have different layouts, so each is routed
// through its own representation while keeping a single call site. The
// serialized form only needs to round-trip within a single engine version
// (shaders are cooked and loaded by the same engine), so the differing byte
// layouts are not a concern.
inline void SerializeOreShaderHash(FArchive& Ar, FOreShaderHash& Hash)
{
#if RIVE_ORE_SHADER_HASH_IS_XXHASH64
    uint64 HashValue = Hash.Hash;
    Ar << HashValue;
    if (Ar.IsLoading())
    {
        Hash = FXxHash64{HashValue};
    }
#else
    Ar << Hash;
#endif
}
