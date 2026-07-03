/*
 * Copyright 2025 Rive
 */

#pragma once

#include "RHI.h"
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/ore/ore_texture.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::ore
{

class OreTextureRHI : public LITE_RTTI_OVERRIDE(Texture, OreTextureRHI)
{
public:
    OreTextureRHI(const TextureDesc& desc);
    virtual ~OreTextureRHI() = default;

    virtual void upload(const TextureDataDesc& data) override;

    FTextureRHIRef m_texture;
};

class OreTextureViewRHI
    : public LITE_RTTI_OVERRIDE(TextureView, OreTextureViewRHI)
{
public:
    OreTextureViewRHI(rcp<Texture> texture, const TextureViewDesc& desc);
    virtual ~OreTextureViewRHI() = default;

    FShaderResourceViewRHIRef m_srv;
};

} // namespace rive::ore
