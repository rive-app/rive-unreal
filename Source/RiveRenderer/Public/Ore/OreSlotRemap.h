/*
 * Copyright 2025 Rive
 */

#pragma once

#include "CoreMinimal.h"

// Translates Ore's per-kind D3D register slots (the "backendSlot": t#, s#, b#)
// to the flat, per-stage descriptor binding index UE's Vulkan RHI expects.
//
// On D3D a resource's bind slot IS its register, so the maps are empty
// and the renderer passes slots straight through. On Vulkan every descriptor
// kind shares one binding namespace per stage, and UE re-numbers them into a
// single sequential array (UBs, then UAVs, then SRVs, then samplers see
// SpirVShaderCompiler.inl). The shader's reflected ParameterMap gives us each
// resource's final index; the cook path builds these maps from it (joined to
// the HLSL register() decorations), and setBindGroup applies them on Vulkan.
struct FOreVulkanSlotRemap
{
    TMap<uint16, uint16> Texture;
    TMap<uint16, uint16> Sampler;
    TMap<uint16, uint16> Uniform;

    bool IsEmpty() const
    {
        return Texture.Num() == 0 && Sampler.Num() == 0 && Uniform.Num() == 0;
    }
};

RIVERENDERER_API FArchive& operator<<(FArchive& Ar, FOreVulkanSlotRemap& Remap);
