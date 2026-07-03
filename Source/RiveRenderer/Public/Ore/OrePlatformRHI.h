// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

// Per-RHI behavior flags for the Ore rendering path, in the style of the GRHI*
// capability globals in RHIGlobals.h: set once at startup and read at the Ore
// code site that depends on each. They default to the desktop-RHI behavior
// (false). A platform whose RHI needs one of these adjustments sets the
// corresponding global to true from its own (out-of-tree) RHI/module init code;
// nothing in this open-source tree enables them.

// The rasterizer's slope-scaled depth bias must be pre-divided by the depth-
// buffer resolution (2^24) because this RHI re-scales it by that factor before
// applying it. Left unadjusted the shadow-map slope bias is vastly too large
// and the depth test rejects every fragment.
extern RIVERENDERER_API bool GRHIOreNeedsSlopeBiasAdjusted;

// The MSAA resolve destination (the Ore canvas) must be created
// ResolveTargetable for this RHI's native hardware resolve to write into it.
// (D3D/Metal don't need it, and D3D12 forbids combining it with
// RenderTargetable, so it stays off there.)
extern RIVERENDERER_API bool GRHIOreNeedsResolveTargetFlag;

// The single-plane shadow depth format (PF_ShadowDepth) is not a robust depth-
// render-target path on this RHI; use the two-plane PF_DepthStencil instead.
extern RIVERENDERER_API bool GRHIOreNeedsDepthStencilShadowFormat;

// This RHI binds shader resources by a flat, per-stage descriptor index taken
// from reflection (the resource's position in the compiled shader's resource
// table) rather than by its HLSL register number, so Ore's D3D-style per-kind
// register slots (t#, s#, b#) must be translated through the reflected
// FOreVulkanSlotRemap before binding. Vulkan needs this too, but is detected
// in-tree via IsVulkanPlatform; any other such RHI sets this global. Leaving it
// false on a flat-binding RHI overruns its descriptor table when a register
// exceeds the table size.
extern RIVERENDERER_API bool GRHIOreNeedsReflectionSlotRemap;

// This RHI compiles the Ore shaders with bindless resources e.g. Metal's
// Shader Converter, which reflects every texture/buffer/sampler through the
// descriptor heap and rejects real register bindings see
// RiveOreShaderHandler's register-strip. Their textures, SRVs and samplers
// must be bound through the bindless parameter setters (SetBindlessTexture /
// SetBindlessResourceView / SetBindlessSampler) so the RHI writes each
// resource's heap handle to its reflected root-constant offset, instead of the
// register-slot setters. The reflected offset is the same value carried in
// FOreVulkanSlotRemap, so this is always paired with
// GRHIOreNeedsReflectionSlotRemap. cbuffers are never made bindless and keep
// the regular uniform-buffer binding. Set this whenever the running platform's
// runtime bindless configuration matches the cook-time one
// (ERHIBindlessConfiguration::All).
extern RIVERENDERER_API bool GRHIOreNeedsBindlessParameters;
