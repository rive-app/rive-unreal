/*
 * Copyright 2025 Rive
 */
#include "TextureRHIImpl.hpp"
#include "TextureResource.h"
#if UE_VERSION_OLDER_THAN(5, 5, 0)

TextureRHIImpl::TextureRHIImpl(const FTextureRHIRef& Texture) :
    rive::gpu::Texture(Texture->GetSizeX(), Texture->GetSizeY()),
    m_texture(Texture)
{}

TextureRHIImpl::TextureRHIImpl(uint32_t width,
                               uint32_t height,
                               uint32_t mipLevelCount,
                               const uint8_t* imageData,
                               EPixelFormat PixelFormat) :
    rive::gpu::Texture(width, height)
{
    FRHIAsyncCommandList commandList;
    FRHICommandListScopedPipelineGuard Guard(*commandList);

    auto Desc = FRHITextureCreateDesc::Create2D(TEXT("rive.PLSTextureRHIImpl_"),
                                                m_width,
                                                m_height,
                                                PixelFormat)
                    .SetNumMips(1);

    m_texture = CREATE_TEXTURE_ASYNC(commandList, Desc);
    commandList->UpdateTexture2D(
        m_texture,
        0,
        FUpdateTextureRegion2D(0, 0, 0, 0, m_width, m_height),
        m_width * 4,
        imageData);
}

FRDGTextureRef TextureRHIImpl::asRDGTexture(FRDGBuilder& Builder) const
{
    check(m_texture);
    return Builder.RegisterExternalTexture(
        CreateRenderTarget(m_texture, TEXT("rive.PLSTextureRHIImpl_")));
}

TextureRHIImpl::~TextureRHIImpl() {}

FTextureRHIRef TextureRHIImpl::contents() const { return m_texture; }

#else // UE VERSION > 5_5:

TextureRHIImpl::TextureRHIImpl(const FTextureRHIRef& InTexture) :
    rive::gpu::Texture(InTexture->GetSizeX(), InTexture->GetSizeY()),
    m_texture(InTexture)
{}

TextureRHIImpl::TextureRHIImpl(const FRDGTextureRef& InTexture) :
    rive::gpu::Texture(InTexture->Desc.GetSize().X,
                       InTexture->Desc.GetSize().Y),
    m_RDGTexture(InTexture)
{}

TextureRHIImpl::TextureRHIImpl(UTextureRenderTarget2D* InTexture) :
    rive::gpu::Texture(InTexture->SizeX, InTexture->SizeY),
    m_UTexture(InTexture)
{}

TextureRHIImpl::TextureRHIImpl(UTexture2D* InTexture) :
    rive::gpu::Texture(InTexture->GetSizeX(), InTexture->GetSizeY()),
    m_UTexture(InTexture)
{}

TextureRHIImpl::TextureRHIImpl(uint32_t width,
                               uint32_t height,
                               uint32_t mipLevelCount,
                               const uint8_t* imageData,
                               EPixelFormat PixelFormat) :
    rive::gpu::Texture(width, height)
{
    FRHICommandList& RHICmdList = GRHICommandList.GetImmediateCommandList();
    FRHICommandListScopedPipelineGuard Guard(RHICmdList);

    auto Desc = FRHITextureCreateDesc::Create2D(TEXT("rive.PLSTextureRHIImpl_"),
                                                m_width,
                                                m_height,
                                                PixelFormat)
                    .SetNumMips(1);

    m_texture = CREATE_TEXTURE_ASYNC(RHICmdList, Desc);
    RHICmdList.UpdateTexture2D(
        m_texture,
        0,
        FUpdateTextureRegion2D(0, 0, 0, 0, m_width, m_height),
        m_width * 4,
        imageData);
}

FRDGTextureRef TextureRHIImpl::asRDGTexture(FRDGBuilder& Builder) const
{
    return m_RDGTexture ? m_RDGTexture
                        : Builder.RegisterExternalTexture(CreateRenderTarget(
                              contents(),
                              TEXT("rive.PLSTextureRHIImpl_")));
}

TextureRHIImpl::~TextureRHIImpl() {}

FTextureRHIRef TextureRHIImpl::contents() const
{
    return m_UTexture.IsValid()
               ? FTextureRHIRef(m_UTexture->GetResource()->GetTexture2DRHI())
               : m_texture;
}

#endif
