/*
 * Copyright 2025 Rive
 */
#pragma once
#include "RenderGraphBuilder.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Logs/RiveRendererLog.h"
#include "RiveShaderTypes.h"
#include "UnrealClient.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/shapes/paint/image_sampler.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/buffer_ring.hpp"
#include "rive/gpu_texture_format.hpp"
THIRD_PARTY_INCLUDES_END

#if UE_VERSION_OLDER_THAN(5, 5, 0)
class TextureRHIImpl : public rive::gpu::Texture
{
public:
    TextureRHIImpl(const FTextureRHIRef& Texture);

    TextureRHIImpl(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevelCount,
                   const uint8_t* imageData,
                   EPixelFormat PixelFormat = PF_B8G8R8A8);

    FRDGTextureRef asRDGTexture(FRDGBuilder& Builder) const;
    virtual ~TextureRHIImpl() override;
    FTextureRHIRef contents() const;

private:
    FTextureRHIRef m_texture;
};
#else // UE VERSION > 5_5:
// FRHIAsyncCommandList was removed in 5.5 we should probably defer load these
// instead but this works for now
class TextureRHIImpl : public rive::gpu::Texture
{
public:
    TextureRHIImpl(const FTextureRHIRef& InTexture);
    TextureRHIImpl(const FRDGTextureRef& InTexture);

    // Variants for data binding render targets.
    TextureRHIImpl(UTextureRenderTarget2D* InTexture);

    // Variant for data binding utextures
    TextureRHIImpl(UTexture2D* InTexture);

    TextureRHIImpl(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevelCount,
                   const uint8_t* imageData,
                   EPixelFormat PixelFormat = PF_B8G8R8A8);

    FRDGTextureRef asRDGTexture(FRDGBuilder& Builder) const;
    virtual ~TextureRHIImpl() override;
    FTextureRHIRef contents() const;

private:
    FRDGTextureRef m_RDGTexture = nullptr;
    FTextureRHIRef m_texture = nullptr;
    TStrongObjectPtr<UTexture> m_UTexture;
};
#endif
